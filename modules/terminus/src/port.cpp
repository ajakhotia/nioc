////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/time.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

fs::path createWorkingDir(const fs::path& logRoot)
{
  fs::create_directories(logRoot);

  const auto name = common::iso8601UtcFormat(std::chrono::system_clock::now()) + "_" +
                    boost::uuids::to_string(boost::uuids::random_generator_pure()());

  const auto dir = logRoot / name;
  fs::create_directories(dir);

  return dir;
}

/// Opens the recording's chronicle writer, or nothing when the run does not record. A run that does
/// not record still publishes — its producers build messages on the heap.
std::unique_ptr<chronicle::Writer> makeWriter(const fs::path& workingDir, const bool record)
{
  if(not record)
  {
    return nullptr;
  }
  const auto dir = workingDir / "chronicle";
  fs::create_directories(dir);
  return std::make_unique<chronicle::Writer>(dir);
}

/// Attaches a file-backed sink to the logger to capture the console log to the working directory.
spdlog::sink_ptr attachLogFileSink(
    const fs::path& consoleLogPath,
    const std::string_view pattern = logger::kDefaultLogPattern)
{
  const spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      consoleLogPath.string(),
      true);

  sink->set_pattern(std::string{pattern});
  logger::addSink(sink);
  return sink;
}

/// Copies the @p source into @p workingDir under its flat basename and records the
/// originalPath → basename mapping in @p fileMap.
void copyResource(
    const fs::path& source,
    const fs::path& workingDir,
    std::unordered_map<std::string, std::string>& fileMap)
{
  if(not fs::exists(source))
  {
    common::throwException<std::invalid_argument>("Resource does not exist: {}", source.string());
  }
  if(not fs::is_regular_file(source))
  {
    common::throwException<std::invalid_argument>(
        "Resource is not a regular file: {}",
        source.string());
  }

  const auto filename = source.filename().string();
  const auto sourceKey = source.string();

  if(fileMap.contains(sourceKey))
  {
    common::throwException<std::invalid_argument>("Resource already added: {}", sourceKey);
  }
  for(const auto& existingFilename: fileMap | std::views::values)
  {
    if(existingFilename == filename)
    {
      common::throwException<std::invalid_argument>(
          "Resource filename collides with a previously added resource: {}",
          filename);
    }
  }

  fs::copy_file(source, workingDir / filename);
  fileMap.emplace(sourceKey, filename);
}

std::unordered_map<std::string, std::string> copyResources(
    const std::vector<fs::path>& resourcePaths,
    const fs::path& workingDir)
{
  auto fileMap = std::unordered_map<std::string, std::string>{};
  for(const auto& resource: resourcePaths)
  {
    copyResource(resource, workingDir, fileMap);
  }
  return fileMap;
}

void writeResources(
    const std::unordered_map<std::string, std::string>& resourceMap,
    const fs::path& workingDir)
{
  const auto path = workingDir / "resources.json";
  auto file = std::ofstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }
  file << nlohmann::json(resourceMap).dump(2) << '\n';
}

} // namespace

Port::Port(Manifest manifest, const Setup& setup):
  mManifest{std::move(manifest)},
  mWorkingDir{createWorkingDir(mManifest.mContext.logRoot())},
  mConsoleLogSink{attachLogFileSink(mWorkingDir / "console.log")},
  mWriter{makeWriter(mWorkingDir, mManifest.mContext.recordChronicle())},
  mLockedResourceMap{copyResources(mManifest.mContext.resourcePaths(), mWorkingDir)}
{
  mManifest.writeTo(mWorkingDir);
  mLockedResourceMap.cExecute([this](const auto& resourceMap)
                              { writeResources(resourceMap, mWorkingDir); });

  logger::debug("recording run to working directory {}", mWorkingDir);

  // Build the routine graph last, so the routines bind to a fully initialized Port.
  std::invoke(setup, *this, mDrivers, mComponents, mRunners);
}

Port::~Port()
{
  // Wind the run down before finalizing the recording: stop the producers, drain the in-flight
  // consignments, then release the routine graph in dependency order. The chronicle writer is
  // released last (by member destruction), after every crate that views its rolls is gone.
  shutdown();
  awaitQuiescence();
  mDrivers.clear();
  mComponents.clear();
  mRunners.clear();

  // The resource map may have grown via addResource() since construction, so rewrite
  // resources.json with its final state. A destructor must not throw, so the write is guarded.
  try
  {
    mLockedResourceMap.cExecute([this](const auto& resourceMap)
                                { writeResources(resourceMap, mWorkingDir); });
  }
  catch(const std::exception& error)
  {
    logger::error("{}", error.what());
  }

  // Detach the file sink last, so any error logged above is still captured in console.log.
  logger::removeSink(mConsoleLogSink);
}

const fs::path& Port::workingDir() const noexcept
{
  return mWorkingDir;
}

const RunContext& Port::runContext() const noexcept
{
  return mManifest.mContext;
}

void Port::addResource(const fs::path& source)
{
  mLockedResourceMap.execute([this, &source](auto& resourceMap)
                             { copyResource(source, mWorkingDir, resourceMap); });
}

fs::path Port::acquireResource(const fs::path& source)
{
  const auto sourceKey = source.string();

  if(const auto resolved = mLockedResourceMap.cExecute(
         [this, &sourceKey](const auto& resourceMap) -> std::optional<fs::path>
         {
           if(const auto entry = resourceMap.find(sourceKey); entry != resourceMap.end())
           {
             return mWorkingDir / entry->second;
           }
           return std::nullopt;
         }))
  {
    return *resolved;
  }

  return mLockedResourceMap.execute(
      [this, &source, &sourceKey](auto& resourceMap)
      {
        if(not resourceMap.contains(sourceKey))
        {
          copyResource(source, mWorkingDir, resourceMap);
        }

        return mWorkingDir / resourceMap.at(sourceKey);
      });
}

fs::path Port::acquireResource(const fs::path& source) const
{
  return mLockedResourceMap.cExecute([this, &source](const auto& resourceMap)
                                     { return mWorkingDir / resourceMap.at(source.string()); });
}

void Port::subscribe(const ChannelId channelId, ConsignmentCallback callback)
{
  mSubscriptionMap[channelId].push_back(std::move(callback));
}

void Port::shutdown() const noexcept
{
  logger::info("Received request to shutdown.");
  static_cast<void>(mShutdownSource.request_stop());
}

void Port::abort() const noexcept
{
  static constexpr std::uint32_t kAbortBit = 0x8000'0000U;
  logger::info("Received request to abort.");
  static_cast<void>(mShutdownSource.request_stop());
  static_cast<void>(mAbortSource.request_stop());
  mPendingConsignments.fetch_or(kAbortBit, std::memory_order_release);
  mPendingConsignments.notify_all();
}

std::stop_token Port::shutdownToken() const noexcept
{
  return mShutdownSource.get_token();
}

std::stop_token Port::abortToken() const noexcept
{
  return mAbortSource.get_token();
}

void Port::awaitQuiescence() const
{
  for(auto pendingConsignments = mPendingConsignments.load(std::memory_order_acquire);
      pendingConsignments > 0 && not mAbortSource.stop_requested();
      pendingConsignments = mPendingConsignments.load(std::memory_order_acquire))
  {
    mPendingConsignments.wait(pendingConsignments, std::memory_order_acquire);
  }
}

bool Port::wait(
    const std::chrono::nanoseconds duration,
    const std::function<void()>& housekeeping) const
{
  const auto deadline = std::chrono::steady_clock::now() + duration;

  std::invoke(housekeeping);

  if(std::ranges::all_of(
         mDrivers,
         [](const auto& driver) { return driver->state() == concurrent::Routine::State::Done; }))
  {
    return false;
  }

  std::this_thread::sleep_until(deadline);
  return true;
}

void Port::deliver(const ChannelId channelId, const chronicle::Crate& crate) const
{
  const auto subscriptions = mSubscriptionMap.find(channelId);
  if(subscriptions == mSubscriptionMap.end())
  {
    return;
  }

  for(const auto& callback: subscriptions->second)
  {
    std::invoke(callback, Consignment{crate, mPendingConsignments});
  }
}

void Port::recordTopic(
    const ChannelId channelId,
    const std::string_view& topic,
    const std::string_view& schemaName)
{
  if(not mRecordedTopics.insert(channelId).second)
  {
    return;
  }

  const auto topicsFilePath = mWorkingDir / "topics.txt";
  auto topicsFile = std::ofstream(topicsFilePath, std::ios::app);
  if(not topicsFile)
  {
    logger::error("Failed to record topic to {}", topicsFilePath.string());
    return;
  }
  topicsFile << topic << '\t' << schemaName << '\n';
}

} // namespace nioc::terminus
