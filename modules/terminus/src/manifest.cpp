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

/// Returns the ordered config file layers @p context and @p variableMap imply: in playback the
/// replayed recording's own config.json comes first, so this invocation's files patch it.
std::vector<fs::path> configLayers(const RunContext& context, const po::variables_map& variableMap)
{
  auto configPaths = std::vector<fs::path>{};
  if(context.playback())
  {
    const auto recordedConfig = context.inputLog() / "config.json";
    if(not fs::is_regular_file(recordedConfig))
    {
      common::throwException<std::invalid_argument>(
          "Not a nioc recording (no config.json): {}",
          context.inputLog().string());
    }
    configPaths.push_back(recordedConfig);
  }

  for(const auto& path: variableMap.at("append-config").as<std::vector<std::string>>())
  {
    configPaths.emplace_back(path);
  }
  return configPaths;
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
      configLayers(mContext, variableMap),
      variableMap.at("config-override").as<std::vector<std::string>>(),
      schema}
{
}

void Manifest::writeTo(const fs::path& recordingDir) const
{
  mConfigStore.writeTo(recordingDir / "config.json");

  auto json = nlohmann::json::object();
  json["cmdline"] = mContext.commandLine();
  json["mode"] = mContext.playback() ? "playback" : "online";
  if(mContext.playback())
  {
    json["inputLog"] = mContext.inputLog().string();
  }

  const auto path = recordingDir / "manifest.json";
  auto file = std::ofstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }
  file << json.dump(2) << '\n';
}

} // namespace nioc::terminus
