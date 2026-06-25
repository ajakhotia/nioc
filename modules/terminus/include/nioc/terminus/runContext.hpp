////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief Read-only snapshot of how one program run was launched: where recordings go, which files
/// to capture, whether to record the chronicle stream, and which recording to replay (if any).
///
/// Build one near startup, then pass it around and query it. The presence of an input log decides
/// the mode: empty means record a new run, non-empty means replay an existing one (see
/// @ref playback). Every accessor is read-only; an instance is safe to read from many threads at
/// once.
///
/// @see cliOptions
class RunContext
{
public:
  /// @brief Return the Boost.ProgramOptions description for the flags this class understands, so a
  /// caller can merge them into its own option set before parsing the command line.
  ///
  /// Defines `--log-root`, `--record-chronicle`, and `--append-resource` (each with a default),
  /// plus the optional `--playback`. Feed the parsed result into the @ref RunContext constructor
  /// that takes a variables_map.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Construct from a parsed option map produced against @ref cliOptions.
  ///
  /// @param variableMap Must contain `log-root`, `record-chronicle`, and `append-resource` (the
  /// @ref cliOptions defaults guarantee this when parsed normally). The optional `playback` and
  /// `commandLine` entries are read only when present.
  ///
  /// @throws std::out_of_range If a required entry is missing.
  ///
  /// @throws boost::bad_any_cast If an entry holds a different type than expected.
  explicit RunContext(const boost::program_options::variables_map& variableMap);

  /// @brief Construct directly from explicit values, bypassing command-line parsing.
  ///
  /// @param logRoot Directory under which this run's new recording is created.
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
  RunContext(
      std::filesystem::path logRoot,
      std::vector<std::filesystem::path> resourcePaths,
      bool recordChronicle,
      std::string commandLine,
      std::filesystem::path inputLog = {});

  /// @brief Directory under which a new recording is created for this run.
  [[nodiscard]] const std::filesystem::path& logRoot() const noexcept;

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

private:
  /// Directory under which this run's new recording is created. Set once at construction.
  std::filesystem::path mLogRoot;

  /// Files to copy into the recording as captured resources. May be empty.
  std::vector<std::filesystem::path> mResourcePaths;

  /// The invocation arguments joined into one string. Empty when none was supplied.
  std::string mCommandLine;

  /// Path of the recording to replay. Empty selects record mode; non-empty selects playback.
  std::filesystem::path mInputLog;

  /// Whether to record the chronicle time-series data stream.
  bool mRecordChronicle;
};

} // namespace nioc::terminus
