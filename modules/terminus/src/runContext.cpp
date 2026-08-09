////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <filesystem>
#include <nioc/common/time.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;
namespace po = boost::program_options;

namespace
{

std::vector<fs::path> pathsFromOption(const po::variables_map& variableMap, const std::string& key)
{
  const auto& values = variableMap.at(key).as<std::vector<std::string>>();
  return {values.begin(), values.end()};
}

fs::path pathFromOption(const po::variables_map& variableMap, const std::string& key)
{
  if(not variableMap.contains(key))
  {
    return {};
  }
  return variableMap.at(key).as<std::string>();
}

std::string commandLineFromOption(const po::variables_map& variableMap)
{
  if(not variableMap.contains("commandLine"))
  {
    return {};
  }
  return variableMap.at("commandLine").as<std::string>();
}

/// Mint a fresh, unique working-directory path under @p logRoot, named by the current UTC instant
/// and a random UUID. The directory is not created here; the constructor creates it.
fs::path mintWorkingDir(const fs::path& logRoot)
{
  const auto name = common::iso8601UtcFormat(std::chrono::system_clock::now()) +
                    "_" +
                    boost::uuids::to_string(boost::uuids::random_generator_pure()());
  return logRoot / name;
}

/// Write `manifest.json` into @p directory: how this run was invoked, and what it replays in
/// playback.
void writeManifest(const RunContext& context, const fs::path& directory)
{
  auto record = nlohmann::json::object();
  record.emplace("cmdline", context.commandLine());
  record.emplace("mode", context.playback() ? "playback" : "online");
  if(context.playback())
  {
    record.emplace("inputLog", context.inputLog().string());
  }
  writeJsonFile(directory / "manifest.json", record);
}

} // namespace

po::options_description RunContext::cliOptions()
{
  auto options = po::options_description("Run context options");

  // clang-format off
  options.add_options()
  (
    "log-root",
    po::value<std::string>()->default_value((fs::temp_directory_path() / "niocLogs").string()),
    "Directory under which a fresh recording is created. Created if missing. "
    "Defaults to <system-temp>/niocLogs"
  )
  (
    "record-chronicle",
    po::value<bool>()->default_value(true),
    "Whether to record the chronicle time-series data stream. Pass false to skip it"
  )
  (
    "append-resource",
    po::value<std::vector<std::string>>()->composing()->default_value({}, ""),
    "File to copy into the recording as a logged resource. Repeat to add more"
  )
  (
    "append-config",
    po::value<std::vector<std::string>>()->composing()->default_value({}, ""),
    "JSON config file to merge. Repeat to add more; files merge left-to-right, so a later "
    "file overrides an earlier one"
  )
  (
    "config-override",
    po::value<std::vector<std::string>>()->composing()->default_value({}, ""),
    "path.to.key=value entry overriding one config value after all files merge. Repeat to add "
    "more; entries apply in order. The value parses as JSON (numbers, bools, arrays, objects), "
    "falls back to a string, and null reverts the value to its schema default"
  )
  (
    "playback",
    po::value<std::string>(),
    "Recording to replay. Selects playback mode and layers the recording's configOverlay.json "
    "beneath this invocation's config files and overrides"
  );
  // clang-format on

  return options;
}

RunContext::RunContext(const po::variables_map& variableMap):
  RunContext{
      mintWorkingDir(variableMap.at("log-root").as<std::string>()),
      pathsFromOption(variableMap, "append-resource"),
      variableMap.at("record-chronicle").as<bool>(),
      commandLineFromOption(variableMap),
      pathFromOption(variableMap, "playback"),
      pathsFromOption(variableMap, "append-config"),
      variableMap.at("config-override").as<std::vector<std::string>>()}
{
}

RunContext::RunContext(
    fs::path workingDir,
    std::vector<fs::path> resourcePaths,
    const bool recordChronicle,
    std::string commandLine,
    fs::path inputLog,
    std::vector<fs::path> appendConfigPaths,
    std::vector<std::string> configOverrides):
  mWorkingDir{std::move(workingDir)},
  mResourcePaths{std::move(resourcePaths)},
  mCommandLine{std::move(commandLine)},
  mInputLog{std::move(inputLog)},
  mAppendConfigPaths{std::move(appendConfigPaths)},
  mConfigOverrides{std::move(configOverrides)},
  mRecordChronicle{recordChronicle},
  mConfigOverlay{mInputLog, mAppendConfigPaths, mConfigOverrides}
{
  fs::create_directories(mWorkingDir);
  mConfigOverlay.write(mWorkingDir);
  writeManifest(*this, mWorkingDir);
}

const fs::path& RunContext::workingDir() const noexcept
{
  return mWorkingDir;
}

const std::vector<fs::path>& RunContext::resourcePaths() const noexcept
{
  return mResourcePaths;
}

bool RunContext::recordChronicle() const noexcept
{
  return mRecordChronicle;
}

const std::string& RunContext::commandLine() const noexcept
{
  return mCommandLine;
}

bool RunContext::playback() const noexcept
{
  return not mInputLog.empty();
}

const fs::path& RunContext::inputLog() const noexcept
{
  return mInputLog;
}

const std::vector<fs::path>& RunContext::appendConfigPaths() const noexcept
{
  return mAppendConfigPaths;
}

const std::vector<std::string>& RunContext::configOverrides() const noexcept
{
  return mConfigOverrides;
}

const ConfigOverlay& RunContext::configOverlay() const noexcept
{
  return mConfigOverlay;
}

} // namespace nioc::terminus
