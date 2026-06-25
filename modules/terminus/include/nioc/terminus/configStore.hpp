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
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief An immutable snapshot of a program's configuration, built by layering JSON config files
/// and command-line overrides and, optionally, decoded against a Cap'n Proto schema for typed
/// access.
///
/// Layers combine in a fixed order: schema defaults (only when a schema is given) first, then each
/// config file left-to-right, then each `key=value` override in order. Every layer is a JSON
/// merge-patch, so a later layer overrides matching keys and a `null` value deletes a key (or,
/// with a schema, reverts it to the schema default). The merged JSON is fixed at construction.
///
/// Example:
///
///     // Schema-bound store: typed access plus JSON dump.
///     ConfigStore store{
///         {"base.json", "override.json"},
///         {"log.level=debug"},
///         capnp::Schema::from<MyConfig>()};
///     auto cfg = store.get<MyConfig>();
///     store.writeTo("effective-config.json");
///
/// Construct with a schema to enable @ref get; without one the store only holds JSON text and
/// supports @ref writeTo. Non-copyable; move-constructible only (not move-assignable). The const
/// accessors may be shared across threads.
///
/// @see cliOptions
class ConfigStore
{
public:
  /// @brief Return the `--append-config` and `--config-override` command-line options that feed the
  /// `configPaths` and `overrides` constructor arguments.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Build a schema-less store from JSON files and overrides, recording only the merged
  /// text. @ref get is unavailable on the result; use @ref writeTo.
  ///
  /// @param configPaths JSON files, read and merge-patched left-to-right.
  ///
  /// @param overrides `path.to.key=value` entries applied after the files, in order. Each value
  /// parses as JSON, falls back to a string, and `null` deletes the key.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  ///
  /// @throws std::invalid_argument if an override is not of the form `path.to.key=value`.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides);

  /// @brief Build a schema-bound store: materialize every `schema` default, layer the files and
  /// overrides over it, then decode the result into a typed Cap'n Proto message for @ref get.
  ///
  /// All defaults are recorded explicitly, so a partial override leaves sibling fields untouched
  /// and `null` reverts a key to its default rather than removing it.
  ///
  /// @param configPaths JSON files, read and merge-patched left-to-right.
  ///
  /// @param overrides `path.to.key=value` entries applied after the files, in order.
  ///
  /// @param schema The Cap'n Proto struct schema; supplies defaults and the decode target.
  /// Unknown keys are rejected.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  ///
  /// @throws std::invalid_argument if an override is malformed.
  ///
  /// @throws kj::Exception if the merged JSON fails to decode against @p schema (e.g.
  /// unknown or ill-typed fields).
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides,
      capnp::StructSchema schema);

  ConfigStore(const ConfigStore&) = delete;

  ConfigStore(ConfigStore&&) noexcept = default;

  ~ConfigStore() = default;

  ConfigStore& operator=(const ConfigStore&) = delete;

  ConfigStore& operator=(ConfigStore&&) noexcept = delete;

  /// @brief Return a typed reader over the decoded config.
  ///
  /// The reader borrows from this store, so it must not outlive the `ConfigStore`.
  ///
  /// @tparam Schema The generated message type matching the `capnp::StructSchema` passed at
  /// construction. Cannot be deduced; specify it explicitly.
  ///
  /// @throws std::logic_error if the store was constructed without a schema.
  template<typename Schema>
  [[nodiscard]] typename Schema::Reader get() const
  {
    if(not mDecodedConfig)
    {
      throw std::logic_error{"ConfigStore holds no decoded config; construct it with a schema"};
    }

    return mDecodedConfig->getRoot<capnp::DynamicStruct>(mSchema).asReader().as<Schema>();
  }

  /// @brief Write the merged config as pretty-printed JSON to @p path, overwriting any existing
  /// file and appending a trailing newline.
  ///
  /// For a schema-bound store every default is present explicitly in the output.
  ///
  /// @param path Destination file.
  ///
  /// @throws std::runtime_error if @p path cannot be opened for writing.
  void writeTo(const std::filesystem::path& path) const;

private:
  /// The fully merged configuration as JSON text, fixed at construction. This is the source for
  /// @ref writeTo and, when a schema is given, for the decode that fills @ref mDecodedConfig.
  std::string mMergedJson;

  /// The Cap'n Proto struct schema used to decode @ref mMergedJson. Default-constructed (empty) for
  /// a schema-less store, in which case @ref mDecodedConfig stays null.
  capnp::StructSchema mSchema;

  /// The typed message decoded from @ref mMergedJson against @ref mSchema, or null when the store
  /// was built without a schema. Backs @ref get.
  std::unique_ptr<capnp::MallocMessageBuilder> mDecodedConfig;
};

} // namespace nioc::terminus
