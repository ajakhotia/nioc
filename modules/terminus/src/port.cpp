////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/sleep.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

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
  writeJsonFile(workingDir / "resources.json", nlohmann::json(resourceMap));
}

} // namespace

Port::Port(RunContext runContext, const Setup& setup):
  mRunContext{std::move(runContext)},
  mConsoleLogSink{attachLogFileSink(mRunContext.workingDir() / "console.log")},
  mWriter{makeWriter(mRunContext.workingDir(), mRunContext.recordChronicle())},
  mLockedResourceMap{copyResources(mRunContext.resourcePaths(), mRunContext.workingDir())},
  mPlaybackTopicRegistry{mRunContext.inputLog()},
  mPlaybackSchemaRegistry{mRunContext.inputLog()}
{
  mLockedResourceMap.cExecute([this](const auto& resourceMap)
                              { writeResources(resourceMap, mRunContext.workingDir()); });

  logger::debug("recording run to working directory {}", mRunContext.workingDir());
  std::invoke(setup, *this, mDrivers, mComponents, mRunners);
  mActiveTopicRegistry.write(mRunContext.workingDir());
  mActiveSchemaRegistry.write(mRunContext.workingDir());
}

Port::~Port() noexcept
{
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
                                { writeResources(resourceMap, mRunContext.workingDir()); });
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
  return mRunContext.workingDir();
}

const RunContext& Port::runContext() const noexcept
{
  return mRunContext;
}

const TopicRegistry& Port::playbackTopics() const noexcept
{
  return mPlaybackTopicRegistry;
}

const SchemaRegistry& Port::playbackSchemas() const noexcept
{
  return mPlaybackSchemaRegistry;
}

void Port::addResource(const fs::path& source)
{
  mLockedResourceMap.execute([this, &source](auto& resourceMap)
                             { copyResource(source, mRunContext.workingDir(), resourceMap); });
}

fs::path Port::acquireResource(const fs::path& source)
{
  const auto sourceKey = source.string();

  if(const auto resolved = mLockedResourceMap.cExecute(
         [this, &sourceKey](const auto& resourceMap) -> std::optional<fs::path>
         {
           if(const auto entry = resourceMap.find(sourceKey); entry != resourceMap.end())
           {
             return mRunContext.workingDir() / entry->second;
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
          copyResource(source, mRunContext.workingDir(), resourceMap);
        }

        return mRunContext.workingDir() / resourceMap.at(sourceKey);
      });
}

fs::path Port::acquireResource(const fs::path& source) const
{
  return mLockedResourceMap.cExecute(
      [this, &source](const auto& resourceMap)
      { return mRunContext.workingDir() / resourceMap.at(source.string()); });
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
  // Acquire is the read side of the handshake with Consignment's release-decrements: this is the
  // only place that reads work guarded by the count reaching zero, so once we observe 0 here, every
  // consumer thread's finished work is visible. Keep this acquire if that invariant ever changes.
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

  // Wait out the poll period, but wake immediately if a shutdown is requested so the run can wind
  // down without sitting out the remaining interval.
  common::sleepUntil(shutdownToken(), deadline);
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

} // namespace nioc::terminus
