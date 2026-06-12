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

/// @brief Everything a Port consumes that originates outside the process: the run's context and
/// its configuration, bound together so they cannot drift apart.
struct Manifest
{
  RunContext mContext;
  ConfigStore mConfigStore;

  /// @brief Returns every command-line option a manifest reads: the run-context section (see @ref
  /// RunContext::cliOptions) and the config section (see @ref ConfigStore::cliOptions).
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Binds an explicitly built context and configuration; tests and embedding code use
  /// this directly.
  ///
  /// @param context The run's per-invocation context.
  ///
  /// @param configStore The run's configuration.
  Manifest(RunContext context, ConfigStore configStore);

  /// @brief Loads the run's manifest from a parsed command line, against a config schema.
  ///
  /// Reads the run context first, then assembles the configuration layers it implies:
  ///
  ///   schema defaults → recording's config.json (playback only) → config files → overrides.
  ///
  /// In playback the replayed recording's own configuration is the base every later layer
  /// patches, so a bare `--playback <recording>` reproduces the recorded setup, and additional
  /// files or overrides tweak it. The result is validated strictly against @p schema and held
  /// canonical and fully explicit (see @ref ConfigStore):
  ///
  /// @code
  /// auto port = Port{Manifest{variableMap, capnp::Schema::from<MyRootConfig>()}, setup};
  /// @endcode
  ///
  /// @param variableMap Parsed options must come from @ref parseCommandLine.
  ///
  /// @param schema Cap'n Proto struct schema describing the binary's full config tree, usually
  /// `capnp::Schema::from<MyRootConfig>()`.
  ///
  /// @throws std::invalid_argument If `--playback` does not name a nioc recording, or an override
  /// is malformed.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  ///
  /// @throws kj::Exception If the merged configuration does not satisfy the @p schema.
  Manifest(const boost::program_options::variables_map& variableMap, capnp::StructSchema schema);

  /// @brief Records the manifest into a recording directory, one file per member.
  ///
  /// Writes the configuration to `config.json` (see @ref ConfigStore::writeTo) and the run
  /// context to `manifest.json` (`cmdline`, `mode`, and — in playback — `inputLog`; values the
  /// recording itself answers, like its own location, are omitted). The manifest is the run's
  /// input record, so both files are written once and never rewritten.
  ///
  /// @param recordingDir The run's working directory.
  ///
  /// @throws std::runtime_error If a file cannot be written.
  void writeTo(const std::filesystem::path& recordingDir) const;
};

} // namespace nioc::terminus
