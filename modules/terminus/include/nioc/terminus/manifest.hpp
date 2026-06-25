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

/// @brief A value that bundles one program invocation's run context with its merged configuration,
/// and can persist both into a recording directory for later replay and audit.
///
/// A program builds one `Manifest` from its parsed command line at startup, then calls @ref writeTo
/// to drop the recording's `config.json` and `manifest.json` into the run's output directory.
///
/// Example:
///
///     auto vm = po::variables_map{};
///     po::store(po::parse_command_line(argc, argv, Manifest::cliOptions()), vm);
///     po::notify(vm);
///     auto manifest = Manifest{vm, MyConfig::schema};
///     manifest.writeTo(recordingDir);
///
/// Non-copyable and not move-assignable (the embedded @ref ConfigStore is move-construct-only);
/// pass it by value to move it.
///
/// @see RunContext, ConfigStore
struct Manifest
{
  /// How this run was invoked: log root, resources, record/playback mode, and command line.
  RunContext mContext;

  /// The layered, optionally schema-decoded configuration for this run.
  ConfigStore mConfigStore;

  /// @brief Return the full set of command-line options the @ref Manifest(const
  /// boost::program_options::variables_map&, capnp::StructSchema) constructor reads.
  ///
  /// Combines `RunContext::cliOptions()` and `ConfigStore::cliOptions()`. Merge this into the
  /// program's option set before parsing, then feed the resulting map to that constructor.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Assemble a manifest from already-built components, moving both into place.
  Manifest(RunContext context, ConfigStore configStore);

  /// @brief Build a manifest from parsed command-line options, decoding the merged config against
  /// @p schema.
  ///
  /// When the run context selects playback, the replayed recording's `config.json` is layered
  /// first, beneath this invocation's `--append-config` files and `--config-override` entries.
  ///
  /// @param variableMap A map parsed against @ref cliOptions(); entries `append-config` and
  /// `config-override` must be present.
  ///
  /// @param schema The Cap'n Proto struct schema that supplies config defaults and the decode
  /// target.
  ///
  /// @throws std::invalid_argument if the playback path holds no `config.json`, or an override is
  /// malformed.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  ///
  /// @throws kj::Exception if the merged JSON does not decode against @p schema.
  Manifest(const boost::program_options::variables_map& variableMap, capnp::StructSchema schema);

  /// @brief Write `config.json` (the merged config) and `manifest.json` (command line plus
  /// online/playback mode, and the input log when replaying) into @p recordingDir, overwriting any
  /// existing files.
  ///
  /// @param recordingDir Destination directory; must already exist.
  ///
  /// @throws std::runtime_error if either file cannot be opened for writing.
  void writeTo(const std::filesystem::path& recordingDir) const;
};

} // namespace nioc::terminus
