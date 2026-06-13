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

/// @brief One run's settings: where it records, what it replays, what files it attaches.
///
/// Read fresh from each command line. Never copied from a replayed recording. The Port records the
/// context into the recording's `manifest.json`.
///
/// Passing `--playback <recording>` selects playback and names the input. Without it, the run is
/// online (live).
class RunContext
{
public:
  /// @brief Returns the options this reads: `--log-root`, `--record-chronicle`,
  /// `--append-resource`, and `--playback`.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Builds the context from a parsed command line (see @ref cliOptions).
  ///
  /// @param variableMap Parsed options. Pass the map from @ref parseCommandLine; it adds the
  /// `"commandLine"` entry this reads.
  explicit RunContext(const boost::program_options::variables_map& variableMap);

  /// @brief Builds the context from explicit values. For tests and embedding code.
  ///
  /// @param logRoot Directory to create the run's fresh recording under.
  ///
  /// @param resourcePaths Files to copy into the recording as logged resources.
  ///
  /// @param recordChronicle True to record the chronicle time-series stream.
  ///
  /// @param commandLine The launch command, stored verbatim in `manifest.json`.
  ///
  /// @param inputLog Recording to replay. Empty means an online run (see @ref playback).
  RunContext(
      std::filesystem::path logRoot,
      std::vector<std::filesystem::path> resourcePaths,
      bool recordChronicle,
      std::string commandLine,
      std::filesystem::path inputLog = {});

  /// @brief Returns the directory to create the run's fresh recording under.
  [[nodiscard]] const std::filesystem::path& logRoot() const noexcept;

  /// @brief Returns the files to copy into the recording.
  [[nodiscard]] const std::vector<std::filesystem::path>& resourcePaths() const noexcept;

  /// @brief Returns true if the run records the chronicle time-series stream.
  [[nodiscard]] bool recordChronicle() const noexcept;

  /// @brief Returns the launch command, verbatim.
  [[nodiscard]] const std::string& commandLine() const noexcept;

  /// @brief Returns true if the run replays a recording instead of running live.
  [[nodiscard]] bool playback() const noexcept;

  /// @brief Returns the recording to replay. Only meaningful when @ref playback is true.
  [[nodiscard]] const std::filesystem::path& inputLog() const noexcept;

private:
  std::filesystem::path mLogRoot;
  std::vector<std::filesystem::path> mResourcePaths;
  std::string mCommandLine;
  std::filesystem::path mInputLog;
  bool mRecordChronicle;
};

} // namespace nioc::terminus
