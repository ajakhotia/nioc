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

/// @brief Holds the merged config and hands out typed read-only views of it.
///
/// The constructor merges the config. Json files merge left to right: a later file overrides an
/// earlier one. Then each `path.to.key=value` override is applied, in command-line order. An
/// override value is parsed as json: numbers, bools, arrays (which replace the whole value), and
/// objects (which merge field by field). Text that is not valid json is used as a string. `null`
/// deletes the key. These are RFC 7386 merge rules.
///
/// @ref writeTo saves the merged tree. With a schema, the constructor also decodes the tree once
/// into a Cap'n Proto message the store owns; views from @ref get stay valid for the store's life.
class ConfigStore
{
public:
  /// @brief Returns the store's command-line options: `--append-config` and `--config-override`.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Builds the store without a schema.
  ///
  /// The store holds only the merged json text. Nothing is decoded, so @ref get cannot be used.
  ///
  /// @param configPaths Json config files, merged left to right; later files override earlier ones.
  ///
  /// @param overrides `path.to.key=value` entries, applied after all files, in order.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file holds invalid json.
  ///
  /// @throws std::invalid_argument If an override is not in `path.to.key=value` form.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides);

  /// @brief Builds the store against a schema, with every default written out.
  ///
  /// The schema's defaults form the bottom layer of the merge; the @p configPaths files and
  /// @p overrides go on top. The result is checked against @p schema. The store then holds the
  /// full text: every key has its effective value, and a key deleted by a `null` override shows
  /// the schema default it fell back to.
  ///
  /// @param configPaths Json config files, merged left to right; later files override earlier ones.
  ///
  /// @param overrides `path.to.key=value` entries, applied after all files, in order.
  ///
  /// @param schema Cap'n Proto struct schema for the binary's full config tree.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file holds invalid json.
  ///
  /// @throws std::invalid_argument If an override is not in `path.to.key=value` form.
  ///
  /// @throws kj::Exception If the merged tree does not match @p schema.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides,
      capnp::StructSchema schema);

  ConfigStore(const ConfigStore&) = delete;

  ConfigStore(ConfigStore&&) noexcept = default;

  ~ConfigStore() = default;

  ConfigStore& operator=(const ConfigStore&) = delete;

  ConfigStore& operator=(ConfigStore&&) noexcept = delete;

  /// @brief Returns the root reader of the decoded config.
  ///
  /// @tparam Schema Compiled type of the schema the store was built against.
  ///
  /// @return Read-only root view. It, and every sub-reader from it, stays valid for the store's
  /// life.
  ///
  /// @throws std::logic_error If the store was built without a schema.
  ///
  /// @throws kj::Exception If @p Schema is not the schema the store was built against.
  template<typename Schema>
  [[nodiscard]] typename Schema::Reader get() const
  {
    if(not mDecodedConfig)
    {
      throw std::logic_error{"ConfigStore holds no decoded config; construct it with a schema"};
    }

    return mDecodedConfig->getRoot<capnp::DynamicStruct>(mSchema).asReader().as<Schema>();
  }

  /// @brief Writes the merged json tree to @p path.
  ///
  /// @param path Output file; overwritten if it exists.
  ///
  /// @throws std::runtime_error If the file cannot be written.
  void writeTo(const std::filesystem::path& path) const;

private:
  std::string mMergedJson;
  capnp::StructSchema mSchema;
  std::unique_ptr<capnp::MallocMessageBuilder> mDecodedConfig;
};

} // namespace nioc::terminus
