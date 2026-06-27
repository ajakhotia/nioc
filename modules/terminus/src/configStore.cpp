////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <boost/program_options.hpp>
#include <capnp/compat/json.h>
#include <capnp/dynamic.h>
#include <capnp/schema.h>
#include <fstream>
#include <nioc/common/exception.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;
namespace po = boost::program_options;

namespace
{

/// Merge the JSON document at the @p path into @p json as a merge-patch.
void mergeJsonFile(nlohmann::json& json, const fs::path& path)
{
  auto file = std::ifstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot open JSON file: {}", path.string());
  }

  json.merge_patch(nlohmann::json::parse(file));
}

/// Apply one `path.to.key=value` assignment to @p json. The left of `=` is a dotted path to the
/// field; the right parses as JSON when it can, otherwise it is taken verbatim as a string, and a
/// `null` value deletes the field. A malformed assignment throws; one naming a field that @p json
/// does not hold is logged and skipped.
void applyAssignment(nlohmann::json& json, const std::string& assignment)
{
  const auto equalsPosition = assignment.find('=');
  if(equalsPosition == std::string::npos or equalsPosition == 0)
  {
    common::throwException<std::invalid_argument>(
        "Assignment must take the form path.to.key=value, got: {}",
        assignment);
  }

  // The dotted path ("log.level") becomes a JSON Pointer to the field ("/log/level").
  auto fieldPointer = "/" + assignment.substr(0, equalsPosition);
  std::ranges::replace(fieldPointer, '.', '/');

  const auto fieldPath = nlohmann::json::json_pointer{fieldPointer};
  if(not json.contains(fieldPath))
  {
    logger::warn("Ignoring assignment for unknown field: {}", assignment);
    return;
  }

  const auto valueText = assignment.substr(equalsPosition + 1);
  const auto value = nlohmann::json::accept(valueText) ? nlohmann::json::parse(valueText)
                                                       : nlohmann::json(valueText);

  overrideField(json, fieldPath, value);
}

/// Assemble a complete JSON document from its sources: layer the @p jsonFilePaths (left-to-right)
/// then the `path.to.key=value` @p assignments (in order) onto @p base as merge-patches, then
/// re-anchor the result on @p base so that a `null` assignment reverts its field to the @p base
/// value rather than dropping it.
nlohmann::json assembleJson(
    nlohmann::json base,
    const std::vector<fs::path>& jsonFilePaths,
    const std::vector<std::string>& assignments)
{
  auto layered = base;
  for(const auto& path: jsonFilePaths)
  {
    mergeJsonFile(layered, path);
  }
  for(const auto& assignment: assignments)
  {
    applyAssignment(layered, assignment);
  }

  base.merge_patch(layered);
  return base;
}

} // namespace

po::options_description ConfigStore::cliOptions()
{
  auto options = po::options_description("Config options");

  // clang-format off
  options.add_options()
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
  );
  // clang-format on

  return options;
}

ConfigStore::ConfigStore(const std::string& json, const capnp::StructSchema schema):
  mSchema{schema},
  mDecodedConfig{decodeMessage(json, schema)}
{
}

ConfigStore::ConfigStore(
    const std::vector<fs::path>& configPaths,
    const std::vector<std::string>& overrides,
    const capnp::StructSchema schema):
  ConfigStore{assembleJson(makeDefaultJson(schema), configPaths, overrides).dump(2), schema}
{
}

void ConfigStore::write(const fs::path& path) const
{
  auto outputFile = std::ofstream(path);
  if(not outputFile)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }

  auto codec = capnp::JsonCodec{};
  codec.setPrettyPrint(true);
  const auto json = codec.encode(mDecodedConfig->getRoot<capnp::DynamicStruct>(mSchema).asReader());
  outputFile << json.cStr() << '\n';
}

} // namespace nioc::terminus
