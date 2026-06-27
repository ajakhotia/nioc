////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief An immutable, schema-decoded snapshot of a program's configuration, for typed access.
///
/// The config is assembled by layering schema defaults, then config files (left-to-right), then
/// `path.to.key=value` overrides — each a JSON merge-patch, so a later layer wins and `null`
/// reverts a field to its schema default. The result is decoded into a typed message, the store's
/// single source of truth that @ref get reads and @ref write emits. Fields outside the schema are
/// ignored, not rejected: a config file may carry them, and an override naming one is logged and
/// skipped.
///
/// Example:
///
///     ConfigStore store{
///         {"base.json", "override.json"}, {"log.level=debug"}, capnp::Schema::from<MyConfig>()};
///     auto cfg = store.get<MyConfig>();
///     store.write("effective-config.json");
///
/// Move-only; not move-assignable. The const accessors may be shared across threads.
///
/// @see cliOptions
class ConfigStore
{
public:
  /// @brief Return the `--append-config` and `--config-override` options feeding the `configPaths`
  /// and `overrides` constructor arguments.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Decode @p json against @p schema — the canonical constructor the others delegate to.
  /// Fields outside @p schema are ignored.
  ///
  /// @param json The configuration as JSON text.
  ///
  /// @param schema The schema to decode against.
  ///
  /// @throws kj::Exception if a field cannot be decoded against @p schema (e.g. a type mismatch).
  ConfigStore(const std::string& json, capnp::StructSchema schema);

  /// @brief Assemble the config — schema defaults, then @p configPaths (left-to-right), then
  /// @p overrides — and delegate to @ref ConfigStore(const std::string&, capnp::StructSchema).
  ///
  /// @param configPaths JSON files, merge-patched left-to-right.
  ///
  /// @param overrides `path.to.key=value` entries applied after the files; `null` reverts a field
  /// to its default, and one naming an off-schema field is logged and skipped.
  ///
  /// @param schema The schema; supplies defaults and the decode target.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  ///
  /// @throws std::invalid_argument if an override is not of the form `path.to.key=value`.
  ///
  /// @throws kj::Exception if a field cannot be decoded against @p schema.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides,
      capnp::StructSchema schema);

  ConfigStore(const ConfigStore&) = delete;

  ConfigStore(ConfigStore&&) noexcept = default;

  ~ConfigStore() = default;

  ConfigStore& operator=(const ConfigStore&) = delete;

  ConfigStore& operator=(ConfigStore&&) noexcept = delete;

  /// @brief Return a typed reader over the decoded config; it borrows from this store and must not
  /// outlive it.
  ///
  /// @tparam Schema The generated message type matching the construction schema; specify
  /// explicitly.
  template<typename Schema>
  [[nodiscard]] Schema::Reader get() const
  {
    return mDecodedConfig->getRoot<capnp::DynamicStruct>(mSchema).asReader().as<Schema>();
  }

  /// @brief Write the decoded config as pretty-printed JSON to @p path (overwriting, trailing
  /// newline). The output is the schema projection — every readable field, nothing off-schema.
  ///
  /// @throws std::runtime_error if @p path cannot be opened for writing.
  void write(const std::filesystem::path& path) const;

private:
  /// The schema the config was decoded against; the type @ref write encodes and @ref get reads.
  capnp::StructSchema mSchema;

  /// The decoded config — the store's single source of truth (null only in a moved-from store).
  std::unique_ptr<capnp::MallocMessageBuilder> mDecodedConfig;
};

} // namespace nioc::terminus
