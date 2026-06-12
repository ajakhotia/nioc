////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>
#include <capnp/compat/json.h>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief Owns the run's merged configuration and serves typed read-only views of it.
///
/// Layers the configuration at construction: json config files merge left-to-right (a later file
/// overrides an earlier one), then each `path.to.key=value` override patches the result in its
/// command-line order. Override values parse as json — numbers, bools, arrays (replacing
/// wholesale), and objects (merging field-wise) all work; unparsable text is taken as a string and
/// `null` deletes the key (RFC 7386 semantics throughout).
///
/// The merged tree is recorded via @ref writeTo and, when a schema is given, decoded once at
/// construction into a Cap'n Proto message the store owns; every reader @ref get serves from it
/// stays valid for the store's lifetime.
class ConfigStore
{
public:
  /// @brief Returns the command-line options the store reads: `--append-config` and
  /// `--config-override`.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Builds the store from explicit layers, without a schema.
  ///
  /// The store holds only the merged json text — nothing is decoded, and @ref get is unavailable.
  ///
  /// @param configPaths Json config files merged left-to-right; later entries override earlier
  /// ones.
  ///
  /// @param overrides `path.to.key=value` entries applied after all files, in order.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  ///
  /// @throws std::invalid_argument If an override lacks the `path.to.key=value` form.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides);

  /// @brief Builds the store against a schema, with every default made explicit.
  ///
  /// Materializes @p schema's defaults into a json tree (the bottom layer of the merge), layers
  /// the @p configPaths files and @p overrides on top, and validates the result strictly against
  /// @p schema. The store holds the canonical, fully-explicit text: every parameter appears with
  /// its effective value, and a key deleted by a `null` override shows the schema default it
  /// reverted to.
  ///
  /// @param configPaths Json config files merged left-to-right; later entries override earlier
  /// ones.
  ///
  /// @param overrides `path.to.key=value` entries applied after all files, in order.
  ///
  /// @param schema Cap'n Proto struct schema describing the binary's full config tree.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  ///
  /// @throws std::invalid_argument If an override lacks the `path.to.key=value` form.
  ///
  /// @throws kj::Exception If the merged tree does not satisfy @p schema.
  ConfigStore(
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::string>& overrides,
      capnp::StructSchema schema);

  ConfigStore(const ConfigStore&) = delete;

  ConfigStore(ConfigStore&&) noexcept = default;

  ~ConfigStore() = default;

  ConfigStore& operator=(const ConfigStore&) = delete;

  ConfigStore& operator=(ConfigStore&&) noexcept = delete;

  /// @brief Returns the root reader of the configuration decoded at construction.
  ///
  /// @tparam Schema Compiled type of the schema the store was constructed against.
  ///
  /// @return Read-only root view of the decoded configuration; it — and every sub-reader obtained
  /// from it — stays valid for the store's lifetime.
  ///
  /// @throws std::logic_error If the store was built without a schema.
  ///
  /// @throws kj::Exception If @p Schema is not the schema the store was constructed against.
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
  /// @param path Destination file; overwritten if present.
  ///
  /// @throws std::runtime_error If the file cannot be written.
  void writeTo(const std::filesystem::path& path) const;

private:
  std::string mMergedJson;
  capnp::StructSchema mSchema;
  std::unique_ptr<capnp::MallocMessageBuilder> mDecodedConfig;
};

} // namespace nioc::terminus
