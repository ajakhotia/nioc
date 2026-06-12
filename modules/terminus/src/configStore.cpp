////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <boost/program_options.hpp>
#include <capnp/compat/json.h>
#include <fstream>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/configStore.hpp>
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

/// Patches the file at @p path into @p config; its entries override existing ones.
void mergeConfigFile(nlohmann::json& config, const fs::path& path)
{
  auto file = std::ifstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot open config file: {}", path.string());
  }

  config.merge_patch(nlohmann::json::parse(file));
}

/// Patches one `path.to.key=value` override into @p config with RFC 7386 semantics: the value
/// parses as json when it can (so `null` deletes, arrays replace, objects merge) and is taken as a
/// string otherwise.
void applyOverride(nlohmann::json& config, const std::string& override)
{
  const auto separator = override.find('=');
  if(separator == std::string::npos || separator == 0)
  {
    common::throwException<std::invalid_argument>(
        "Config override must take the form path.to.key=value, got: {}",
        override);
  }

  auto pointerText = "/" + override.substr(0, separator);
  std::ranges::replace(pointerText, '.', '/');

  const auto valueText = override.substr(separator + 1);
  const auto value = nlohmann::json::accept(valueText) ? nlohmann::json::parse(valueText)
                                                       : nlohmann::json(valueText);

  auto patch = nlohmann::json::object();
  patch[nlohmann::json::json_pointer{pointerText}] = value;
  config.merge_patch(patch);
}

/// Patches the @p configPaths files (left-to-right) and then the @p overrides (in order) into
/// @p tree and returns it.
nlohmann::json layeredTree(
    nlohmann::json tree,
    const std::vector<fs::path>& configPaths,
    const std::vector<std::string>& overrides)
{
  for(const auto& path: configPaths)
  {
    mergeConfigFile(tree, path);
  }
  for(const auto& override: overrides)
  {
    applyOverride(tree, override);
  }
  return tree;
}

/// Builds the json tree of @p schema's defaults: the bottom layer every other config layer
/// patches. Reading an untouched message yields each field's default — struct literals included —
/// so the tree is assembled by walking such readers and encoding every field explicitly.
nlohmann::json defaultsTree(const capnp::StructSchema schema, const capnp::JsonCodec& codec)
{
  auto message = capnp::MallocMessageBuilder{};
  const auto root = message.initRoot<capnp::DynamicStruct>(schema).asReader();

  struct Node
  {
    capnp::DynamicStruct::Reader mReader;
    nlohmann::json::json_pointer mPath;
  };

  auto tree = nlohmann::json::object();
  auto pending = std::vector<Node>{};
  pending.push_back(Node{.mReader = root, .mPath = nlohmann::json::json_pointer{}});
  while(not pending.empty())
  {
    const auto node = pending.back();
    pending.pop_back();

    for(const auto& field: node.mReader.getSchema().getFields())
    {
      const auto path = node.mPath / field.getProto().getName().cStr();
      if(field.getType().isStruct())
      {
        tree[path] = nlohmann::json::object();
        pending.push_back(
            Node{.mReader = node.mReader.get(field).as<capnp::DynamicStruct>(), .mPath = path});
      }
      else
      {
        tree[path] = nlohmann::json::parse(
            codec.encode(node.mReader.get(field), field.getType()).cStr());
      }
    }
  }

  return tree;
}

/// Returns the canonical, fully-explicit config text: @p schema's defaults patched by the
/// @p configPaths files and @p overrides, with a key a `null` override deleted restored to its
/// default.
std::string canonicalConfigText(
    const std::vector<fs::path>& configPaths,
    const std::vector<std::string>& overrides,
    const capnp::StructSchema schema)
{
  const auto codec = capnp::JsonCodec{};
  const auto defaults = defaultsTree(schema, codec);

  auto canonical = defaults;
  canonical.merge_patch(layeredTree(defaults, configPaths, overrides));
  return canonical.dump(2);
}

/// Decodes @p mergedJson strictly against @p schema — a key naming no schema field fails — and
/// returns the message holding the result.
std::unique_ptr<capnp::MallocMessageBuilder> decodedConfig(
    const std::string& mergedJson,
    const capnp::StructSchema schema)
{
  auto codec = capnp::JsonCodec{};
  codec.setRejectUnknownFields(true);

  auto message = std::make_unique<capnp::MallocMessageBuilder>();
  codec.decode(
      kj::ArrayPtr<const char>{mergedJson.data(), mergedJson.size()},
      message->initRoot<capnp::DynamicStruct>(schema));
  return message;
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
    "more; entries apply in order. The value parses as json (numbers, bools, arrays, objects), "
    "falls back to a string, and null deletes the key"
  );
  // clang-format on

  return options;
}

ConfigStore::ConfigStore(
    const std::vector<fs::path>& configPaths,
    const std::vector<std::string>& overrides):
  mMergedJson{layeredTree(nlohmann::json::object(), configPaths, overrides).dump(2)}
{
}

// mMergedJson is declared before mDecodedConfig, so the canonical text is ready by the time the
// decode consumes it.
ConfigStore::ConfigStore(
    const std::vector<fs::path>& configPaths,
    const std::vector<std::string>& overrides,
    const capnp::StructSchema schema):
  mMergedJson{canonicalConfigText(configPaths, overrides, schema)},
  mSchema{schema},
  mDecodedConfig{decodedConfig(mMergedJson, schema)}
{
}

void ConfigStore::writeTo(const fs::path& path) const
{
  auto file = std::ofstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }
  file << mMergedJson << '\n';
}

} // namespace nioc::terminus
