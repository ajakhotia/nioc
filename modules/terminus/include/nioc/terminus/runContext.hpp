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

/// @brief The per-invocation context of a run: where it records, what it replays, what it attaches.
///
/// A run's context is read fresh from each command line and — unlike the configuration — is never
/// inherited from a replayed recording: no run wants another run's log root or input. The Port
/// records the context into the recording's `metadata.json`.
///
/// The mode is implied rather than stated: passing `--playback <recording>` selects playback and
/// names the input in one stroke; its absence means an online run.
class RunContext
{
public:
  /// @brief Returns the command-line options the context reads: `--log-root`,
  /// `--record-chronicle`, `--append-resource`, and `--playback`.
  [[nodiscard]] static boost::program_options::options_description cliOptions();

  /// @brief Reads the context from a parsed command line (see @ref cliOptions).
  ///
  /// @param variableMap Parsed options; must come from @ref parseCommandLine, which adds the
  /// `"commandLine"` entry this constructor reads.
  explicit RunContext(const boost::program_options::variables_map& variableMap);

  /// @brief Builds a context from explicit values; tests and embedding code use this directly.
  ///
  /// @param logRoot Directory under which the run's fresh recording is created.
  ///
  /// @param resourcePaths Files copied into the recording as logged resources.
  ///
  /// @param recordChronicle When true, record the chronicle time-series data stream.
  ///
  /// @param commandLine The verbatim launch command, recorded in `metadata.json`.
  ///
  /// @param inputLog Recording to replay; empty selects an online run (see @ref playback).
  RunContext(
      std::filesystem::path logRoot,
      std::vector<std::filesystem::path> resourcePaths,
      bool recordChronicle,
      std::string commandLine,
      std::filesystem::path inputLog = {});

  /// @brief Returns the directory under which the run's fresh recording is created.
  [[nodiscard]] const std::filesystem::path& logRoot() const noexcept;

  /// @brief Returns the files to copy into the recording as logged resources.
  [[nodiscard]] const std::vector<std::filesystem::path>& resourcePaths() const noexcept;

  /// @brief Returns whether the run records the chronicle time-series data stream.
  [[nodiscard]] bool recordChronicle() const noexcept;

  /// @brief Returns the verbatim launch command.
  [[nodiscard]] const std::string& commandLine() const noexcept;

  /// @brief Returns true when the run replays a recording instead of running live.
  [[nodiscard]] bool playback() const noexcept;

  /// @brief Returns the recording being replayed; meaningful only when @ref playback is true.
  [[nodiscard]] const std::filesystem::path& inputLog() const noexcept;

private:
  std::filesystem::path mLogRoot;
  std::vector<std::filesystem::path> mResourcePaths;
  std::string mCommandLine;
  std::filesystem::path mInputLog;
  bool mRecordChronicle;
};

} // namespace nioc::terminus
