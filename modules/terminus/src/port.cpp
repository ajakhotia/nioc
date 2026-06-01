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
#include <nioc/terminus/port.hpp>
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

/// Bound on the dispatcher and recorder queues. Both use OverflowPolicy::Block, so a full queue
/// applies backpressure to publishers rather than dropping messages.
constexpr auto kQueueCapacity = std::size_t{ 1024 };

fs::path createWorkingDir(const fs::path& logRoot)
{
  fs::create_directories(logRoot);

  const auto name = common::iso8601UtcFormat(std::chrono::system_clock::now()) + "_" +
                    boost::uuids::to_string(boost::uuids::random_generator_pure()());

  const auto dir = logRoot / name;
  fs::create_directories(dir);

  return dir;
}

std::unique_ptr<chronicle::Writer> makeChronicleWriter(const fs::path& workingDir)
{
  const auto chronicleDir = workingDir / "chronicle";
  fs::create_directories(chronicleDir);
  return std::make_unique<chronicle::Writer>(chronicleDir);
}

/// Extracts a repeatable path option (absent → empty), preserving command-line order.
std::vector<fs::path> pathsFromOption(const po::variables_map& variableMap, const std::string& key)
{
  if(not variableMap.contains(key))
  {
    return {};
  }
  const auto& values = variableMap.at(key).as<std::vector<std::string>>();
  return { values.begin(), values.end() };
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

/// Attach a file-backed sink to the logger to capture the console log to the working directory
spdlog::sink_ptr attachLogFileSink(
    const fs::path& consoleLogPath,
    const std::string_view pattern = logger::kDefaultLogPattern)
{
  const spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      consoleLogPath.string(),
      true);

  sink->set_pattern(std::string{ pattern });
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
    std::string commandLine):
    mWorkingDir{ createWorkingDir(logRoot) },
    mConsoleLogSink{ attachLogFileSink(mWorkingDir / "console.log") },
    mConfig(loadAndWriteConfig(configPaths, mWorkingDir)),
    mResourceMap{ copyResources(resourcePaths, mWorkingDir) },
    mChronicleWriter{ writeChronicle ? makeChronicleWriter(mWorkingDir) : nullptr }
{
  auto metadata = nlohmann::json::object();
  metadata["cmdline"] = commandLine;
  metadata["resources"] = mResourceMap;
  writeJsonFile(mWorkingDir / "metadata.json", metadata);

  // Bring the recorder up before the dispatcher, so the dispatcher always has somewhere to tee to.
  if(mChronicleWriter)
  {
    mRecorder = std::make_shared<AsyncProcessor<std::pair<ChannelId, ConstMsgBasePtr>>>(
        kQueueCapacity,
        OverflowPolicy::Block,
        [this](const auto item)
        {
          write(*item.second, item.first, *mChronicleWriter);
        });
    mRecorderRunner = std::make_shared<ThreadedRunner>();
    mRecorderRunner->launch(mRecorder);
  }

  mDispatcher = std::make_shared<AsyncProcessor<std::pair<ChannelId, ConstMsgBasePtr>>>(
      kQueueCapacity,
      OverflowPolicy::Block,
      [this](const auto item)
      {
        dispatch(item.first, item.second);
      });
  mDispatcherRunner = std::make_shared<ThreadedRunner>();
  mDispatcherRunner->launch(mDispatcher);
}

Port::~Port()
{
  // Stop the dispatcher first so it stops feeding the recorder, then stop the recorder. Keeping the
  // recorder running until the dispatcher has joined lets any dispatcher push that is blocked on a
  // full recorder queue complete. Stopping is abrupt: messages still queued are dropped, which a
  // planned drain-on-close pass (shared with Component) will address.
  if(mDispatcherRunner)
  {
    mDispatcherRunner->requestStop();
    mDispatcherRunner->waitUntilStopped();
  }
  if(mRecorderRunner)
  {
    mRecorderRunner->requestStop();
    mRecorderRunner->waitUntilStopped();
  }

  // Resources may have grown via addResource() since construction, so refresh the resources
  // section of the metadata written in the constructor. A destructor must not throw, so the
  // metadata update is guarded.
  try
  {
    const auto metadataPath = mWorkingDir / "metadata.json";
    if(auto file = std::fstream(metadataPath, std::ios::in | std::ios::out))
    {
      auto metadata = nlohmann::json::parse(file);
      metadata["resources"] = mResourceMap;

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
  copyResource(source, mWorkingDir, mResourceMap);
}

fs::path Port::acquireResource(const fs::path& source)
{
  if(not mResourceMap.contains(source.string()))
  {
    copyResource(source, mWorkingDir, mResourceMap);
  }

  return mWorkingDir / mResourceMap.at(source.string());
}

fs::path Port::acquireResource(const fs::path& source) const
{
  return mWorkingDir / mResourceMap.at(source.string());
}

void Port::subscribe(const ChannelId channelId, std::weak_ptr<const MsgCallback> callbackPtr)
{
  const auto lock = std::scoped_lock(mSubscriptionMutex);
  mSubscriptionMap[channelId].emplace_back(std::move(callbackPtr));
}

void Port::publish(const ChannelId channelId, ConstMsgBasePtr msgPtr)
{
  mDispatcher->push_back(std::make_pair(channelId, std::move(msgPtr)));
}

void Port::dispatch(const ChannelId channelId, const ConstMsgBasePtr& msgBasePtr)
{
  {
    const auto lock = std::scoped_lock(mSubscriptionMutex);
    if(mSubscriptionMap.contains(channelId))
    {
      for(const auto& weakCallback: mSubscriptionMap.at(channelId))
      {
        const auto callback = weakCallback.lock();
        if(not callback)
        {
          continue;
        }

        std::invoke(*callback, msgBasePtr);
      }
    }
  }

  if(mRecorder)
  {
    mRecorder->push_back(std::make_pair(channelId, std::move(msgBasePtr)));
  }
}

} // namespace nioc::terminus
