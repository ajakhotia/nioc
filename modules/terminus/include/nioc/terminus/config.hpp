////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/message.h>
#include <capnp/schema.h>
#include <capnp/serialize.h>
#include <cstddef>
#include <filesystem>
#include <nioc/containers/mmapConstArray.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace nioc::terminus
{

namespace detail
{

/// @brief One instance's config, materialized on disk and memory-mapped, held untyped.
///
/// Carries the non-template machinery behind @ref Config: it merges the instance overrides onto a
/// schema's defaults, decodes the result, writes `<directory>/<name>.json` (the effective config
/// as the schema projection) and `<directory>/<name>.bin` (the same message as a single-segment
/// flat-array frame), and maps the binary read-only. The schema arrives as a runtime
/// `capnp::StructSchema` so none of this depends on the config type; @ref Config supplies the type
/// at the @ref getRoot call.
///
/// Not copyable; move-construction transfers the mapping and re-roots over its (address-stable)
/// bytes, leaving the source fit only for destruction. This is an implementation detail of
/// @ref Config, not public API.
class MappedConfig
{
public:
  /// @brief Materialize the effective config from @p overrides and write and map its artifacts.
  ///
  /// @param overrides Instance overrides, merge-patched onto the schema defaults. Must be a JSON
  /// object; pass an empty object for a defaults-only config.
  ///
  /// @param directory Directory the `<name>.json` and `<name>.bin` artifacts are written to;
  /// created if absent.
  ///
  /// @param name Basename of the artifacts, without extension.
  ///
  /// @param schema Supplies the defaults and the decode target.
  ///
  /// @throws std::invalid_argument if @p overrides is not a JSON object.
  ///
  /// @throws std::runtime_error if an artifact cannot be written or mapped.
  MappedConfig(
      const nlohmann::json& overrides,
      const std::filesystem::path& directory,
      const std::string& name,
      capnp::StructSchema schema);

  MappedConfig(const MappedConfig&) = delete;

  /// @brief Take over @p other's mapping and re-root over it, leaving @p other fit only for
  /// destruction.
  MappedConfig(MappedConfig&& other) noexcept;

  ~MappedConfig() noexcept = default;

  MappedConfig& operator=(const MappedConfig&) = delete;

  MappedConfig& operator=(MappedConfig&&) = delete;

  /// @brief Return the message root read as @p Schema; it borrows from this object and must not
  /// outlive it.
  ///
  /// Non-const because the underlying reader builds its arena lazily on first root access; call it
  /// once and keep the returned reader.
  ///
  /// @tparam Schema The generated Cap'n Proto struct type the config was materialized against.
  template<typename Schema>
  [[nodiscard]] Schema::Reader getRoot()
  {
    return mFlatMessageReader.getRoot<Schema>();
  }

private:
  // Declared in this order so construction reads the message in place from the mapped bytes.

  /// Owns the memory-mapped bytes of `<name>.bin` that back every reader this object hands out.
  containers::MmapConstArray<std::byte> mMappedConfigArray;

  /// The Cap'n Proto message read in place from @ref mMappedConfigArray's bytes.
  capnp::FlatArrayMessageReader mFlatMessageReader;

  /// @brief Root over an already-materialized @p mappedConfigArray: the funnel the public and
  /// move constructors delegate to.
  explicit MappedConfig(containers::MmapConstArray<std::byte> mappedConfigArray);
};

} // namespace detail

/// @brief One instance's typed configuration: the schema's defaults merged with the instance's
/// JSON overrides, decoded, persisted beside the run, and read through a memory-mapped view.
///
/// Construction does all the work: it merges @p overrides onto the schema defaults, decodes the
/// result, writes the effective config as `<directory>/<name>.json` (human-readable record) and
/// `<directory>/<name>.bin` (single-segment flat-array frame), maps the binary read-only, and
/// roots a reader in the mapping, so the values read at runtime and the artifact on disk are the
/// same bytes.
///
/// Example:
///
///     RoadBuilder::RoadBuilder(std::string name, Port& port, Config<RoadBuilderConfig> config):
///       Component{std::move(name), port, config.reader().getComponent()},
///       mConfig{std::move(config)}
///     {
///     }
///
/// A Config is owned by the routine it configures and taken by value. Not copyable;
/// move-construction transfers the mapping, leaving the source fit only for destruction. Immutable
/// after construction, so concurrent reads are safe.
///
/// @tparam Schema_ The generated Cap'n Proto struct type of this instance's config.
///
/// @see detail::MappedConfig for the on-disk artifact contract.
template<typename Schema_>
class Config
{
public:
  using Schema = Schema_;

  /// @brief Materialize the effective config from @p overrides into @p directory under @p name,
  /// and root a reader in the mapped binary.
  ///
  /// @param overrides Instance overrides, merge-patched onto the schema defaults. Must be a JSON
  /// object; pass an empty object for a defaults-only config.
  ///
  /// @param directory Directory the `<name>.json` and `<name>.bin` artifacts are written to;
  /// created if absent.
  ///
  /// @param name Basename of the artifacts, without extension.
  ///
  /// @throws std::invalid_argument if @p overrides is not a JSON object.
  ///
  /// @throws std::runtime_error if an artifact cannot be written or mapped.
  Config(
      const nlohmann::json& overrides,
      const std::filesystem::path& directory,
      const std::string& name):
    mMappedConfig{overrides, directory, name, capnp::Schema::from<Schema>()},
    mReader{mMappedConfig.getRoot<Schema>()}
  {
  }

  Config(const Config&) = delete;

  Config(Config&&) noexcept = default;

  ~Config() noexcept = default;

  Config& operator=(const Config&) = delete;

  Config& operator=(Config&&) = delete;

  /// @brief Return a reader over the decoded config; it borrows from this Config and must not
  /// outlive it.
  [[nodiscard]] Schema::Reader reader() const noexcept
  {
    return mReader;
  }

private:
  // mMappedConfig is declared first so mReader can be read out of its mapped bytes at construction.

  /// The mapped, decoded config backing @ref mReader.
  detail::MappedConfig mMappedConfig;

  /// The typed reader over @ref mMappedConfig; the value @ref reader returns.
  Schema::Reader mReader;
};

} // namespace nioc::terminus
