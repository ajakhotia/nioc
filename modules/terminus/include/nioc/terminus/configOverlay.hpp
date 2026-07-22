////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nioc::terminus
{

/// @brief One run's assembled configuration: the sparse per-routine overrides that layer on top of
/// each schema's defaults, plus the one place that knows how a config document is shaped.
///
/// Construction assembles the document by layering, left to right: the replayed recording's
/// overrides (playback only), then each `--append-config` file, then each `--config-override`
/// assignment. Each layer is a JSON merge-patch, so a later layer wins. The result is the sparse
/// overrides document, not a materialized config; schema defaults are filled in per routine by
/// @ref Config.
///
/// A document is sectioned for humans and for the launcher that reads it:
/// `{routines: {components: {<name>: overrides}, drivers: {<name>: overrides}}}`. A routine draws
/// its slice by name alone and never names its section; @ref acquireOverrides looks the name up
/// across both. Construction rejects a name that appears in more than one section: a routine name
/// identifies exactly one routine.
///
/// A value type: assembled and validated at construction, immutable in practice, and cheap to move.
///
/// @see RunContext, Config, Port::materializeConfig
class ConfigOverlay
{
public:
  /// @brief Assemble the overrides document from its layers and validate its routine names.
  ///
  /// @param inputLog Empty in record mode. In playback, the recording whose `configOverlay.json`
  /// is layered beneath this run's files and overrides.
  ///
  /// @param appendConfigPaths Config files merged left-to-right, so a later file wins over an
  /// earlier one.
  ///
  /// @param configOverrides `path.to.key=value` assignments applied after the files, in order.
  ///
  /// @throws std::invalid_argument if @p inputLog is set but holds no `configOverlay.json`, if an
  /// override assignment is malformed, or if a routine name appears in more than one section.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error if a config file is not valid JSON.
  ConfigOverlay(
      const std::filesystem::path& inputLog,
      const std::vector<std::filesystem::path>& appendConfigPaths,
      const std::vector<std::string>& configOverrides);

  ConfigOverlay(const ConfigOverlay&) = default;

  ConfigOverlay(ConfigOverlay&&) noexcept = default;

  ~ConfigOverlay() = default;

  ConfigOverlay& operator=(const ConfigOverlay&) = default;

  ConfigOverlay& operator=(ConfigOverlay&&) noexcept = default;

  /// @brief The whole assembled overrides document, sectioned as launched.
  [[nodiscard]] const nlohmann::json& document() const noexcept;

  /// @brief Persist the document as `configOverlay.json` under @p directory, the same filename a
  /// later run reads back when it replays this one.
  ///
  /// @param directory An existing directory to write into.
  ///
  /// @throws std::runtime_error if the file cannot be opened for writing.
  void write(const std::filesystem::path& directory) const;

  /// @brief The override blob for the routine named @p name, or an empty object if the document
  /// carries no entry for it.
  ///
  /// A routine passes this to its @ref Config, which merges it onto the schema defaults, so an
  /// absent entry means the routine runs on pure defaults. The lookup is by name across both
  /// sections; the caller never names a section.
  [[nodiscard]] nlohmann::json acquireOverrides(const std::string& name) const;

private:
  /// The assembled, sectioned overrides document: the single source of truth. @ref acquireOverrides
  /// looks a routine up in it by name; construction validates that no name spans both sections.
  nlohmann::json mDocument;
};

} // namespace nioc::terminus
