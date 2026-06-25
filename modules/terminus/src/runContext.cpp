////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <filesystem>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
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
    "playback",
    po::value<std::string>(),
    "Recording to replay. Selects playback mode and layers the recording's config.json "
    "beneath this invocation's config files and overrides"
  );
  // clang-format on

  return options;
}

RunContext::RunContext(const po::variables_map& variableMap):
  RunContext{
      variableMap.at("log-root").as<std::string>(),
      pathsFromOption(variableMap, "append-resource"),
      variableMap.at("record-chronicle").as<bool>(),
      commandLineFromOption(variableMap),
      pathFromOption(variableMap, "playback")}
{
}

RunContext::RunContext(
    fs::path logRoot,
    std::vector<fs::path> resourcePaths,
    const bool recordChronicle,
    std::string commandLine,
    fs::path inputLog):
  mLogRoot{std::move(logRoot)},
  mResourcePaths{std::move(resourcePaths)},
  mCommandLine{std::move(commandLine)},
  mInputLog{std::move(inputLog)},
  mRecordChronicle{recordChronicle}
{
}

const fs::path& RunContext::logRoot() const noexcept
{
  return mLogRoot;
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

} // namespace nioc::terminus
