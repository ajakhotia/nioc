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
#include <mutex>
#include <nioc/common/exception.hpp>
#include <nioc/common/time.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/asyncChronicleWriter.hpp>
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
///
/// @throws std::invalid_argument If the @p source is missing, is not a regular file, was already
/// added, or its basename collides with an already-copied resource.
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

/// Copies every resource in @p resourcePaths into @p workingDir and returns the resulting
/// originalPath → basename map.
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

/// Writes the originalPath → basename map of the recording's resource copies to resources.json.
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
  mLockedResourceMap{copyResources(mManifest.mContext.resourcePaths(), mWorkingDir)},
  mChronicleWriter{
      mManifest.mContext.recordChronicle()
          ? std::make_unique<AsyncChronicleWriter>(mWorkingDir / "chronicle")
          : nullptr}
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
  // consignments, then release the routine graph in dependency order — drivers stop producing,
  // components have nothing left to consume, and runners join their threads once the routines
  // they drive expire.
  shutdown();
  awaitQuiescence();
  mDrivers.clear();
  mComponents.clear();
  mRunners.clear();

  // Stop the chronicle writer next: it joins its thread, so no further chronicle write or trace
  // log races the resources update and log-sink removal below. A no-op when this run does not
  // record.
  mChronicleWriter.reset();

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

  // Shared-lock fast check to acquire existing resources.
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

  // Exclusive-lock path to copy the resource. The copy may happen at most once.
  return mLockedResourceMap.execute(
      [this, &source, &sourceKey](auto& resourceMap)
      {
        // Recheck for a potential race since the lock was released in the block above.
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
  mLockedSubscriptionMap.execute([channelId, &callback](auto& subscriptionMap)
                                 { subscriptionMap[channelId].push_back(std::move(callback)); });
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

void Port::publish(const ChannelId channelId, const ConstMsgBasePtr& msgBasePtr)
{
  mLockedSubscriptionMap.cExecute(
      [this, channelId, &msgBasePtr](const auto& subscriptionMap)
      {
        if(const auto subscriptions = subscriptionMap.find(channelId);
           subscriptions != subscriptionMap.end())
        {
          for(const auto& callback: subscriptions->second)
          {
            std::invoke(callback, Consignment{msgBasePtr, mPendingConsignments});
          }
        }
      });

  if(mChronicleWriter)
  {
    mChronicleWriter->push(channelId, Consignment{msgBasePtr, mPendingConsignments});
  }
}

void Port::recordTopic(
    const ChannelId channelId,
    const std::string_view& topic,
    const std::string_view& schemaName)
{
  // Shared fast check if a topic has already been recorded.
  if(mLockedChannelIdSet.cExecute([channelId](const auto& channelIdSet)
                                  { return channelIdSet.contains(channelId); }))
  {
    return;
  }

  // Record the topic if it hasn't been recorded yet.
  mLockedChannelIdSet.execute(
      [this, channelId, &topic, &schemaName](auto& channelIdSet)
      {
        // Recheck to avoid race condition since having relieved the block above.
        if(channelIdSet.contains(channelId))
        {
          return;
        }

        // Best effort to record the topic to the file.
        const auto topicsFilePath = mWorkingDir / "topics.txt";
        auto topicsFile = std::ofstream(topicsFilePath, std::ios::app);
        if(not topicsFile)
        {
          logger::error("Failed to record topic to {}", topicsFilePath.string());
          return;
        }
        topicsFile << topic << '\t' << schemaName << '\n';

        channelIdSet.emplace(channelId);
      });
}

} // namespace nioc::terminus
