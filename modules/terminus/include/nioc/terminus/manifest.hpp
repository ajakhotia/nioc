////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "configStore.hpp"
#include "runContext.hpp"
#include <boost/program_options.hpp>
#include <filesystem>

namespace nioc::terminus
{

/// @brief A Port's outside-the-process input: the run context plus its configuration.
struct Manifest
{
  RunContext mContext;
  ConfigStore mConfigStore;

  /// @brief Returns the command-line options a manifest reads: the run-context options (@ref
  /// RunContext::cliOptions) plus the config options (@ref ConfigStore::cliOptions).
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Builds a manifest from a ready context and config. For tests and embedding.
  ///
  /// @param context The run context.
  ///
  /// @param configStore The configuration.
  Manifest(RunContext context, ConfigStore configStore);

  /// @brief Builds a manifest from parsed command-line options and a config schema.
  ///
  /// Layers the configuration in this order, each one patching the last:
  ///
  ///   schema defaults → recording's config.json (playback only) → config files → overrides.
  ///
  /// In playback the recording's config is the base, so a bare `--playback <recording>` reproduces
  /// the recorded setup; extra files or overrides change it. The result is validated against @p
  /// schema and stored canonical and fully explicit (@ref ConfigStore).
  ///
  /// @code
  /// auto port = Port{Manifest{variableMap, capnp::Schema::from<MyRootConfig>()}, setup};
  /// @endcode
  ///
  /// @param variableMap Parsed options. Must come from @ref parseCommandLine.
  ///
  /// @param schema Cap'n Proto schema for the full config tree, usually
  /// `capnp::Schema::from<MyRootConfig>()`.
  ///
  /// @throws std::invalid_argument If `--playback` does not name a nioc recording, or an override
  /// is malformed.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file holds malformed JSON.
  ///
  /// @throws kj::Exception If the merged configuration does not match @p schema.
  Manifest(const boost::program_options::variables_map& variableMap, capnp::StructSchema schema);

  /// @brief Writes the manifest to a recording directory, one file per member.
  ///
  /// Writes the config to `config.json` (@ref ConfigStore::writeTo) and the run context to
  /// `manifest.json` (`cmdline`, `mode`, and — in playback — `inputLog`; values the recording
  /// already answers, like its own location, are skipped). Both files are written once and never
  /// rewritten.
  ///
  /// @param recordingDir The run's working directory.
  ///
  /// @throws std::runtime_error If a file cannot be written.
  void writeTo(const std::filesystem::path& recordingDir) const;
};

} // namespace nioc::terminus
