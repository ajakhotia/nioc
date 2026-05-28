////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>
#include <filesystem>
#include <memory>
#include <nioc/chronicle/writer.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace spdlog::sinks
{
class sink;
} // namespace spdlog::sinks

namespace nioc::terminus
{

/// @brief Central data hub for a nioc binary.
///
/// One Port instance owns the on-disk recording for one run of a nioc binary. It creates the
/// recording directory, captures the launch command, loads and merges the JSON configuration,
/// adds a file sink to the nioc default logger so the run's log output is also written into the
/// recording, and opens the chronicle writer that downstream code uses to record time-series data.
/// On destruction, it detaches that file sink and finalizes the recording by writing
/// `metadata.json` (the verbatim launch command plus the resource manifest).
class Port
{
public:
  /// @brief Constructs a Port from a parsed command line.
  ///
  /// Reads Port's own options and the `"commandLine"` entry (see @ref programOptions and
  /// @ref parseCommandLine) out of @p variableMap.
  ///
  /// @param variableMap Parsed options; must come from @ref parseCommandLine, which adds the
  /// `"commandLine"` entry this constructor reads.
  ///
  /// @throws std::invalid_argument If a listed resource is missing, not a regular file, or
  /// collides.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  explicit Port(const boost::program_options::variables_map& variableMap);

  /// @brief Constructs a Port and creates a fresh recording directory.
  ///
  /// The recording directory is named `<iso8601>_<uuid>` and lives directly under @p logRoot,
  /// which is created if it does not exist. The run's log output is also written to `console.log`
  /// inside it; the merged configuration is written to `config.json`; each resource is copied in;
  /// and, when @p writeChronicle is true, chronicle data lands in `chronicle/`.
  ///
  /// @param logRoot Directory under which the fresh recording is created. Created if missing.
  ///
  /// @param configPaths JSON config files merged left-to-right; later entries override earlier
  /// ones.
  ///
  /// @param resourcePaths Files copied into the recording as logged resources (see @ref
  /// addResource).
  ///
  /// @param writeChronicle When true, record the chronicle time-series data stream under
  /// `chronicle/`.
  ///
  /// @param commandLine The verbatim launch command, recorded in `metadata.json`.
  ///
  /// @throws std::invalid_argument If a resource is missing, not a regular file, or collides.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  Port(
      const std::filesystem::path& logRoot,
      const std::vector<std::filesystem::path>& configPaths,
      const std::vector<std::filesystem::path>& resourcePaths,
      bool writeChronicle,
      std::string commandLine);

  Port(const Port&) = delete;

  Port(Port&&) noexcept = delete;

  ~Port();

  Port& operator=(const Port&) = delete;

  Port& operator=(Port&&) noexcept = delete;

  /// @brief Returns the working directory that holds this run's recording.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// @brief Returns the merged JSON configuration.
  [[nodiscard]] const nlohmann::json& config() const noexcept;

  /// @brief Adds a supporting file to the recording.
  ///
  /// Copies the file at the @p source location into the recording directory under its filename. The
  /// mapping `originalPath → filename` is recorded in `metadata.json` at shutdown.
  ///
  /// @param source Path to the file being added.
  ///
  /// @throws std::invalid_argument If the @p source does not exist, is not a regular file, or its
  /// filename collides with a previously added resource.
  void addResource(const std::filesystem::path& source);

  /// @brief Resolves a previously added resource to its copy inside the working directory.
  ///
  /// @p source is the original path that was passed to @ref addResource. The returned path points
  /// at the flat copy that addResource made under the working directory, so callers read from the
  /// self-contained recording rather than the resource's original location.
  ///
  /// @param source The original path previously passed to @ref addResource.
  ///
  /// @return The path to the resource's copy inside the working directory.
  ///
  /// @throws std::invalid_argument If @p source was not previously added (see @ref addResource).
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

private:
  const std::filesystem::path mWorkingDir;

  /// File sink for this run's `console.log`. Attached to the nioc default logger for the Port's
  /// lifetime and detached in the destructor.
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;

  const nlohmann::json mConfig;
  const std::string mCommandLine;
  std::unordered_map<std::string, std::string> mResourceMap;
  const std::unique_ptr<chronicle::Writer> mChronicleWriter;
};

} // namespace nioc::terminus
