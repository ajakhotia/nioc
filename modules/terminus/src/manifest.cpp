////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/manifest.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;
namespace po = boost::program_options;

namespace
{

/// Collect the config files to layer, in merge order: the replayed recording's `config.json` first
/// (playback only), then each `--append-config` file.
std::vector<fs::path> configFileLayers(
    const RunContext& context,
    const po::variables_map& variableMap)
{
  auto configFilePaths = std::vector<fs::path>{};
  if(context.playback())
  {
    const auto recordedConfig = context.inputLog() / "config.json";
    if(not fs::is_regular_file(recordedConfig))
    {
      common::throwException<std::invalid_argument>(
          "Not a nioc recording (no config.json): {}",
          context.inputLog().string());
    }
    configFilePaths.push_back(recordedConfig);
  }

  for(const auto& appendConfigPath: variableMap.at("append-config").as<std::vector<std::string>>())
  {
    configFilePaths.emplace_back(appendConfigPath);
  }
  return configFilePaths;
}

/// Build the `manifest.json` record: how this run was invoked, and what it replays when in
/// playback.
nlohmann::json manifestRecord(const RunContext& context)
{
  auto record = nlohmann::json::object();
  record.emplace("cmdline", context.commandLine());
  record.emplace("mode", context.playback() ? "playback" : "online");
  if(context.playback())
  {
    record.emplace("inputLog", context.inputLog().string());
  }
  return record;
}

/// Write @p json as pretty-printed text to @p path, with a trailing newline.
void writeJsonFile(const fs::path& path, const nlohmann::json& json)
{
  auto outputFile = std::ofstream(path);
  if(not outputFile)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }
  outputFile << json.dump(2) << '\n';
}

} // namespace

po::options_description Manifest::cliOptions()
{
  auto options = po::options_description{};
  options.add(RunContext::cliOptions());
  options.add(ConfigStore::cliOptions());
  return options;
}

Manifest::Manifest(RunContext context, ConfigStore configStore):
  mContext{std::move(context)},
  mConfigStore{std::move(configStore)}
{
}

// mContext is declared before mConfigStore, so it is fully built by the time the config layering
// consults it for the playback input.
Manifest::Manifest(const po::variables_map& variableMap, const capnp::StructSchema schema):
  mContext{variableMap},
  mConfigStore{
      configFileLayers(mContext, variableMap),
      variableMap.at("config-override").as<std::vector<std::string>>(),
      schema}
{
}

void Manifest::write(const fs::path& recordingDir) const
{
  mConfigStore.write(recordingDir / "config.json");
  writeJsonFile(recordingDir / "manifest.json", manifestRecord(mContext));
}

} // namespace nioc::terminus
