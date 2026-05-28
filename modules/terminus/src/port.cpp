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
#include <nioc/common/exception.hpp>
#include <nioc/common/time.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/port.hpp>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <unordered_map>

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
/// <workingDir>/config.json, and returns it — so the config member is initialized from the same
/// value that was recorded to disk.
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

/// Creates the console.log file sink and attaches it to the nioc default logger, so log events
/// are captured from this point on. Returns it so the Port can detach it on destruction.
spdlog::sink_ptr setupConsoleLogSink(const fs::path& consoleLogPath)
{
  const spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      consoleLogPath.string(),
      true);

  logger::addSink(sink);
  return sink;
}

/// Copies @p source into @p workingDir under its flat basename and records the
/// originalPath → basename mapping in @p fileMap.
///
/// @throws std::invalid_argument If @p source is missing, is not a regular file, was already
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
/// originalPath → basename map, used to initialize the Port's resource map member.
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
    const std::vector<fs::path>& configPaths, // NOLINT(bugprone-easily-swappable-parameters)
    const std::vector<fs::path>& resourcePaths, // NOLINT(bugprone-easily-swappable-parameters)
    const bool writeChronicle,
    std::string commandLine):
    mWorkingDir{ createWorkingDir(logRoot) },
    mConsoleLogSink{ setupConsoleLogSink(mWorkingDir / "console.log") },
    mConfig(loadAndWriteConfig(configPaths, mWorkingDir)),
    mCommandLine{ std::move(commandLine) },
    mResourceMap{ copyResources(resourcePaths, mWorkingDir) },
    mChronicleWriter{ writeChronicle ? makeChronicleWriter(mWorkingDir) : nullptr }
{
}

Port::~Port()
{
  try
  {
    auto metadata = nlohmann::json::object();
    metadata["cmdline"] = mCommandLine;
    metadata["resources"] = mResourceMap;
    writeJsonFile(mWorkingDir / "metadata.json", metadata);
  }
  catch(const std::exception& error)
  {
    // Destructor must not throw. Log and move on so the recording is still finalized as best
    // we can; chronicle::Writer's destructor will flush whatever it has.
    logger::error("Failed to write metadata.json: {}", error.what());
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

fs::path Port::acquireResource(const fs::path& source) const
{
  const auto entry = mResourceMap.find(source.string());
  if(entry == mResourceMap.end())
  {
    common::throwException<std::invalid_argument>(
        "Resource {} was not declared.",
        source.string());
  }

  return mWorkingDir / entry->second;
}

} // namespace nioc::terminus
