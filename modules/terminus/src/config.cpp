////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/any.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <capnp/serialize.h>
#include <cstddef>
#include <filesystem>
#include <nioc/common/exception.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <nioc/terminus/config.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace nioc::terminus::detail
{
namespace fs = std::filesystem;

namespace
{

/// Materialize @p overrides against @p schema into `<directory>/<name>.json` (the effective config)
/// and `<directory>/<name>.bin` (a bare single-segment flat-array frame), and return the binary
/// mapped read-only. @p name names both artifacts.
containers::MmapConstArray<std::byte> materialize(
    const nlohmann::json& overrides,
    const fs::path& directory,
    const std::string& name,
    const capnp::StructSchema schema)
{
  if(not overrides.is_object())
  {
    common::throwException<std::invalid_argument>(
        "Config overrides must be a JSON object, got: {}",
        overrides.dump());
  }

  auto configJson = encodeAsJson(schema);
  configJson.merge_patch(overrides);
  const auto materializedConfig = decodeFromJson(configJson.dump(), schema);

  fs::create_directories(directory);

  // The effective config: every field resolved to its final value, a readable record of what this
  // instance ran with.
  writeJsonFile(directory / (name + ".json"), encodeAsJson(*materializedConfig, schema));

  const auto binPath = directory / (name + ".bin");
  flattenAndWrite(*materializedConfig, binPath);

  // The write mapping is closed above; the finished frame reopens read-only for the config's
  // lifetime view.
  return containers::MmapConstArray<std::byte>{binPath};
}

} // namespace

MappedConfig::MappedConfig(
    const nlohmann::json& overrides,
    const fs::path& directory,
    const std::string& name,
    const capnp::StructSchema schema):
  MappedConfig{materialize(overrides, directory, name, schema)}
{
}

MappedConfig::MappedConfig(MappedConfig&& other) noexcept:
  MappedConfig{std::move(other.mMappedConfigArray)}
{
}

MappedConfig::MappedConfig(containers::MmapConstArray<std::byte> mappedConfigArray):
  mMappedConfigArray{std::move(mappedConfigArray)},
  mFlatMessageReader{
      asWords(std::span<const std::byte>{mMappedConfigArray.data(), mMappedConfigArray.size()})}
{
}

} // namespace nioc::terminus::detail
