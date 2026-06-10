////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
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
#include <nioc/terminus/port.hpp>
#include <optional>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;
namespace po = boost::program_options;

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

/// Extracts a repeatable path option (absent → empty), preserving command-line order.
std::vector<fs::path> pathsFromOption(const po::variables_map& variableMap, const std::string& key)
{
  if(not variableMap.contains(key))
  {
    return {};
  }
  const auto& values = variableMap.at(key).as<std::vector<std::string>>();
  return {values.begin(), values.end()};
}

void writeJsonFile(const fs::path& path, const nlohmann::json& json)
{
  auto file = std::ofstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }
  file << json.dump(2) << '\n';
}

/// Merges @p configPaths left-to-right (later files override earlier ones), writes the result to
/// config.json in @p workingDir, and returns it.
nlohmann::json loadAndWriteConfig(
    const std::vector<fs::path>& configPaths,
    const fs::path& workingDir)
{
  auto config = nlohmann::json::object();
  for(const auto& path: configPaths)
  {
    auto file = std::ifstream(path);
    if(not file)
    {
      common::throwException<std::runtime_error>("Cannot open config file: {}", path.string());
    }

    config.merge_patch(nlohmann::json::parse(file));
  }

  writeJsonFile(workingDir / "config.json", config);
  return config;
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

} // namespace

Port::Port(const po::variables_map& variableMap):
  Port(
      variableMap.at("log-root").as<std::string>(),
      pathsFromOption(variableMap, "append-config"),
      pathsFromOption(variableMap, "append-resource"),
      variableMap.at("record-chronicle").as<bool>(),
      variableMap.at("commandLine").as<std::string>())
{
}

Port::Port(
    const fs::path& logRoot,
    const std::vector<fs::path>& configPaths,   // NOLINT(bugprone-easily-swappable-parameters)
    const std::vector<fs::path>& resourcePaths, // NOLINT(bugprone-easily-swappable-parameters)
    const bool writeChronicle,
    const std::string& commandLine):
  mWorkingDir{createWorkingDir(logRoot)},
  mConsoleLogSink{attachLogFileSink(mWorkingDir / "console.log")},
  mConfig(loadAndWriteConfig(configPaths, mWorkingDir)),
  mLockedResourceMap{copyResources(resourcePaths, mWorkingDir)},
  mChronicleWriter{
      writeChronicle ? std::make_unique<AsyncChronicleWriter>(mWorkingDir / "chronicle") : nullptr}
{
  auto metadata = nlohmann::json::object();
  metadata["cmdline"] = commandLine;
  mLockedResourceMap.cExecute([&metadata](const auto& resourceMap)
                              { metadata["resources"] = resourceMap; });
  writeJsonFile(mWorkingDir / "metadata.json", metadata);

  logger::debug("recording run to working directory {}", mWorkingDir);
}

Port::~Port()
{
  // Stop the chronicle writer first: it joins its thread, so no further chronicle write or trace
  // log races the metadata update and log-sink removal below. A no-op when this run does not
  // record.
  mChronicleWriter.reset();

  // Resources may have grown via addResource() since construction, so refresh the resources
  // section of the metadata written in the constructor. A destructor must not throw, so the
  // metadata update is guarded.
  try
  {
    const auto metadataPath = mWorkingDir / "metadata.json";
    if(auto file = std::fstream(metadataPath, std::ios::in | std::ios::out))
    {
      auto metadata = nlohmann::json::parse(file);
      mLockedResourceMap.cExecute([&metadata](const auto& resourceMap)
                                  { metadata["resources"] = resourceMap; });

      file.clear();
      file.seekp(0);
      file << metadata.dump(2) << '\n';
    }
    else
    {
      logger::error("Failed to update metadata.json: cannot open {}", metadataPath.string());
    }
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

const nlohmann::json& Port::config() const noexcept
{
  return mConfig;
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
