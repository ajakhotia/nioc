////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "configOverlay.hpp"
#include <boost/program_options.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief The identity of one program run: the working directory it owns, the config layers it was
/// launched with, its captured resources, whether it records a chronicle, and which recording it
/// replays (if any).
///
/// Build one near startup and hand it to a @ref Port. The convenience constructor mints and creates
/// a fresh working directory under the log root; the explicit constructor takes the working
/// directory as given (and creates it), which is what tests use for a deterministic location. The
/// presence of an input log decides the mode: empty means record a new run, non-empty means replay
/// an existing one (see @ref playback).
///
/// Construction fully establishes the run on disk: it mints or takes the working directory and
/// creates it, assembles this run's @ref ConfigOverlay from the config layers, and writes the
/// run's `configOverlay.json` and `manifest.json` into the working directory. A constructed context
/// carries a config that is already merged and validated.
///
/// Move-only, and consumed by exactly one Port: a run owns its working directory, so a moved-from
/// context is spent and fit only for destruction. Every accessor is read-only; an instance is safe
/// to read from many threads at once.
///
/// @see cliOptions, ConfigOverlay
class RunContext
{
public:
  /// @brief Return the Boost.ProgramOptions description for the flags this class understands, so a
  /// caller can merge them into its own option set before parsing the command line.
  ///
  /// Defines `--log-root`, `--record-chronicle`, `--append-resource`, `--append-config`, and
  /// `--config-override` (each with a default), plus the optional `--playback`. Feed the parsed
  /// result into the @ref RunContext constructor that takes a variables_map.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Construct from a parsed option map produced against @ref cliOptions, minting and
  /// creating a fresh working directory under `--log-root`.
  ///
  /// @param variableMap Must contain `log-root`, `record-chronicle`, `append-resource`,
  /// `append-config`, and `config-override` (the @ref cliOptions defaults guarantee this when
  /// parsed normally). The optional `playback` and `commandLine` entries are read only when
  /// present.
  ///
  /// @throws std::out_of_range If a required entry is missing.
  ///
  /// @throws boost::bad_any_cast If an entry holds a different type than expected.
  explicit RunContext(const boost::program_options::variables_map& variableMap);

  /// @brief Construct directly from explicit values, bypassing command-line parsing, and create
  /// @p workingDir.
  ///
  /// @param workingDir The directory this run owns; created if absent.
  ///
  /// @param resourcePaths Files to copy into the recording as captured resources; may be empty.
  ///
  /// @param recordChronicle Whether to record the chronicle time-series data stream.
  ///
  /// @param commandLine The invocation arguments joined into one string; empty if none was
  /// supplied.
  ///
  /// @param inputLog Leave empty to record a new run; set it to replay that recording. Defaults to
  /// empty (record mode).
  ///
  /// @param appendConfigPaths Config files layered onto the document left-to-right. Defaults to
  /// none.
  ///
  /// @param configOverrides `path.to.key=value` entries applied after the files. Defaults to none.
  RunContext(
      std::filesystem::path workingDir,
      std::vector<std::filesystem::path> resourcePaths,
      bool recordChronicle,
      std::string commandLine,
      std::filesystem::path inputLog = {},
      std::vector<std::filesystem::path> appendConfigPaths = {},
      std::vector<std::string> configOverrides = {});

  RunContext(const RunContext&) = delete;

  RunContext(RunContext&&) noexcept = default;

  ~RunContext() = default;

  RunContext& operator=(const RunContext&) = delete;

  RunContext& operator=(RunContext&&) noexcept = delete;

  /// @brief The directory this run owns; holds its chronicle, console log, config, and resources.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// @brief Files to copy into the recording as captured resources; may be empty.
  [[nodiscard]] const std::vector<std::filesystem::path>& resourcePaths() const noexcept;

  /// @brief Whether to record the chronicle time-series data stream.
  [[nodiscard]] bool recordChronicle() const noexcept;

  /// @brief The invocation arguments joined into one string; empty if none was supplied.
  [[nodiscard]] const std::string& commandLine() const noexcept;

  /// @brief Whether this run replays a recording instead of producing a new one.
  ///
  /// @return True exactly when @ref inputLog is non-empty.
  [[nodiscard]] bool playback() const noexcept;

  /// @brief Path of the recording to replay; empty in record mode.
  [[nodiscard]] const std::filesystem::path& inputLog() const noexcept;

  /// @brief Config files layered onto the document left-to-right; may be empty.
  [[nodiscard]] const std::vector<std::filesystem::path>& appendConfigPaths() const noexcept;

  /// @brief `path.to.key=value` entries applied after the files; may be empty.
  [[nodiscard]] const std::vector<std::string>& configOverrides() const noexcept;

  /// @brief This run's assembled configuration: the sparse per-routine overrides, ready for a
  /// routine to draw its slice by name.
  [[nodiscard]] const ConfigOverlay& configOverlay() const noexcept;

private:
  /// The directory this run owns. Set once at construction and created there.
  std::filesystem::path mWorkingDir;

  /// Files to copy into the recording as captured resources. May be empty.
  std::vector<std::filesystem::path> mResourcePaths;

  /// The invocation arguments joined into one string. Empty when none was supplied.
  std::string mCommandLine;

  /// Path of the recording to replay. Empty selects record mode; non-empty selects playback.
  std::filesystem::path mInputLog;

  /// Config files layered onto the document left-to-right. May be empty.
  std::vector<std::filesystem::path> mAppendConfigPaths;

  /// `path.to.key=value` entries applied after the config files. May be empty.
  std::vector<std::string> mConfigOverrides;

  /// Whether to record the chronicle time-series data stream.
  bool mRecordChronicle;

  /// This run's assembled overrides. Declared last so it is built from the config-layer members
  /// above, which are already initialized by the time its construction assembles and validates it.
  ConfigOverlay mConfigOverlay;
};

} // namespace nioc::terminus
