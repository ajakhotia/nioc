////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <filesystem>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/configOverlay.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

// The document's shape lives here, and only here: routines split into a components and a drivers
// section, each an object mapping a routine name to its override blob.
constexpr auto kRoutinesKey = "routines";
constexpr auto kComponentsSection = "components";
constexpr auto kDriversSection = "drivers";

// The overlay's on-disk filename, shared by the playback read and the persisting write so a run and
// the run that replays it agree on it.
constexpr auto kOverlayFileName = "configOverlay.json";

/// Apply one `path.to.key=value` assignment to @p document, creating the field if absent. The left
/// of `=` is a dotted path; the right parses as JSON when it can, otherwise it is taken verbatim as
/// a string. A `null` value is stored as-is and reverts the field to its schema default when the
/// routine is later decoded.
void applyOverride(nlohmann::json& document, const std::string& assignment)
{
  const auto equalsPosition = assignment.find('=');
  if(equalsPosition == std::string::npos or equalsPosition == 0)
  {
    common::throwException<std::invalid_argument>(
        "Assignment must take the form path.to.key=value, got: {}",
        assignment);
  }

  // The dotted path ("routines.drivers.hiroHills.miningTimeMs") becomes a JSON Pointer.
  auto fieldPointer = "/" + assignment.substr(0, equalsPosition);
  std::ranges::replace(fieldPointer, '.', '/');

  const auto valueText = assignment.substr(equalsPosition + 1);
  const auto value = nlohmann::json::accept(valueText) ? nlohmann::json::parse(valueText)
                                                       : nlohmann::json(valueText);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  document[nlohmann::json::json_pointer{fieldPointer}] = value;
}

/// Assemble the sparse overrides document by layering the recorded overrides (playback only), then
/// the append-config files left-to-right, then the config-override assignments.
nlohmann::json assemble(
    const fs::path& inputLog,
    const std::vector<fs::path>& appendConfigPaths,
    const std::vector<std::string>& configOverrides)
{
  auto document = nlohmann::json::object();

  if(not inputLog.empty())
  {
    const auto recordedOverlay = inputLog / kOverlayFileName;
    if(not fs::is_regular_file(recordedOverlay))
    {
      common::throwException<std::invalid_argument>(
          "Not a nioc recording (no {}): {}",
          kOverlayFileName,
          inputLog.string());
    }
    document.merge_patch(readJsonFile(recordedOverlay));
  }

  for(const auto& appendConfigPath: appendConfigPaths)
  {
    document.merge_patch(readJsonFile(appendConfigPath));
  }

  for(const auto& assignment: configOverrides)
  {
    applyOverride(document, assignment);
  }

  return document;
}

/// Throw if any routine name appears in both sections; a routine name must identify exactly one
/// routine. A name absent from the document, or present in a single section, is fine.
void requireUniqueRoutineNames(const nlohmann::json& document)
{
  const auto routines = document.find(kRoutinesKey);
  if(routines == document.end())
  {
    return;
  }

  const auto components = routines->find(kComponentsSection);
  const auto drivers = routines->find(kDriversSection);
  if(components == routines->end() or
     not components->is_object() or
     drivers == routines->end() or
     not drivers->is_object())
  {
    return;
  }

  for(const auto& component: components->items())
  {
    if(drivers->contains(component.key()))
    {
      common::throwException<std::invalid_argument>(
          "Routine name {} appears in more than one config section.",
          component.key());
    }
  }
}

} // namespace

ConfigOverlay::ConfigOverlay(
    const fs::path& inputLog,
    const std::vector<fs::path>& appendConfigPaths,
    const std::vector<std::string>& configOverrides):
  mDocument(assemble(inputLog, appendConfigPaths, configOverrides))
{
  requireUniqueRoutineNames(mDocument);
}

const nlohmann::json& ConfigOverlay::document() const noexcept
{
  return mDocument;
}

void ConfigOverlay::write(const fs::path& directory) const
{
  writeJsonFile(directory / kOverlayFileName, mDocument);
}

nlohmann::json ConfigOverlay::acquireOverrides(const std::string& name) const
{
  // A routine is looked up by name across both sections; construction has already ruled out a name
  // living in more than one, so the first hit is the only hit.
  const auto routines = mDocument.find(kRoutinesKey);
  if(routines == mDocument.end())
  {
    return nlohmann::json::object();
  }

  for(const auto* const sectionKey: {kComponentsSection, kDriversSection})
  {
    const auto section = routines->find(sectionKey);
    if(section == routines->end() or not section->is_object())
    {
      continue;
    }

    if(const auto entry = section->find(name); entry != section->end())
    {
      return *entry;
    }
  }

  return nlohmann::json::object();
}

} // namespace nioc::terminus
