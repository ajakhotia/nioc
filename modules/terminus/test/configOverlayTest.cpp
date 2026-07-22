////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/terminus/configOverlay.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

fs::path testDirectory(const fs::path& name)
{
  return fs::temp_directory_path() / "niocConfigOverlayTest" / name;
}

fs::path writeConfigFile(const fs::path& name, const std::string& text)
{
  const auto path = testDirectory(name);
  fs::create_directories(path.parent_path());
  std::ofstream(path) << text;
  return path;
}

/// Stage a recording directory holding a `configOverlay.json`, as playback reads it.
fs::path makeRecording(const fs::path& name, const std::string& overlayText)
{
  const auto dir = testDirectory(name);
  fs::create_directories(dir);
  std::ofstream(dir / "configOverlay.json") << overlayText;
  return dir;
}

} // namespace

TEST(ConfigOverlayTest, layersFilesLeftToRightThenOverrides)
{
  const auto base = writeConfigFile(
      "base.json",
      R"({"routines": {"drivers": {"hiroHills": {"miningTimeMs": 1, "resourceTopic": "brick"}}}})");
  const auto overlay = writeConfigFile(
      "overlay.json",
      R"({"routines": {"drivers": {"hiroHills": {"miningTimeMs": 2}}}})");

  const auto overrides =
      ConfigOverlay{{}, {base, overlay}, {"routines.drivers.hiroHills.miningTimeMs=3"}};
  const auto& document = overrides.document();

  // The later file wins over the earlier, and the override wins over both. A field only the base
  // set survives.
  EXPECT_EQ(
      document.at(nlohmann::json::json_pointer{"/routines/drivers/hiroHills/miningTimeMs"}),
      3);
  EXPECT_EQ(
      document.at(nlohmann::json::json_pointer{"/routines/drivers/hiroHills/resourceTopic"}),
      "brick");
}

TEST(ConfigOverlayTest, overrideCreatesAnAbsentPath)
{
  const auto overrides = ConfigOverlay{{}, {}, {"routines.drivers.hiroHills.miningTimeMs=5"}};

  EXPECT_EQ(
      overrides.document().at(
          nlohmann::json::json_pointer{"/routines/drivers/hiroHills/miningTimeMs"}),
      5);
}

TEST(ConfigOverlayTest, playbackLayersRecordedOverlayBeneathThisRun)
{
  const auto recording = makeRecording(
      "replayed",
      R"({"routines": {"drivers": {"hiroHills": {"miningTimeMs": 9, "resourceTopic": "brick"}}}})");

  const auto overrides =
      ConfigOverlay{recording, {}, {"routines.drivers.hiroHills.miningTimeMs=500"}};
  const auto& document = overrides.document();

  // The recording pins resourceTopic; this run's override outranks the recorded miningTimeMs.
  EXPECT_EQ(
      document.at(nlohmann::json::json_pointer{"/routines/drivers/hiroHills/miningTimeMs"}),
      500);
  EXPECT_EQ(
      document.at(nlohmann::json::json_pointer{"/routines/drivers/hiroHills/resourceTopic"}),
      "brick");
}

TEST(ConfigOverlayTest, playbackRejectsNonRecording)
{
  EXPECT_THROW((ConfigOverlay{testDirectory("noSuchRecording"), {}, {}}), std::invalid_argument);
}

TEST(ConfigOverlayTest, overridesLookupIsByNameAcrossSections)
{
  const auto config = writeConfigFile(
      "byName.json",
      R"({"routines": {
            "drivers": {"hiroHills": {"miningTimeMs": 7}},
            "components": {"rohanTheRoadBuilder": {"brickPerRoad": 2}}}})");

  const auto overlay = ConfigOverlay{{}, {config}, {}};

  // A routine draws its slice by name alone, whichever section it lives in.
  EXPECT_EQ(overlay.acquireOverrides("hiroHills").at("miningTimeMs"), 7);
  EXPECT_EQ(overlay.acquireOverrides("rohanTheRoadBuilder").at("brickPerRoad"), 2);
}

TEST(ConfigOverlayTest, acquireOverridesReturnsEmptyForUnknownRoutine)
{
  const auto overlay = ConfigOverlay{{}, {}, {}};
  EXPECT_TRUE(overlay.acquireOverrides("neverConfigured").empty());
}

TEST(ConfigOverlayTest, rejectsNameInMoreThanOneSection)
{
  const auto config = writeConfigFile(
      "collision.json",
      R"({"routines": {
            "drivers": {"twin": {"miningTimeMs": 1}},
            "components": {"twin": {"brickPerRoad": 1}}}})");

  EXPECT_THROW((ConfigOverlay{{}, {config}, {}}), std::invalid_argument);
}

TEST(ConfigOverlayTest, writePersistsTheDocument)
{
  const auto overrides = ConfigOverlay{{}, {}, {"routines.drivers.hiroHills.miningTimeMs=4"}};
  const auto directory = testDirectory("persisted");
  fs::create_directories(directory);

  overrides.write(directory);

  const auto onDisk = nlohmann::json::parse(std::ifstream(directory / "configOverlay.json"));
  EXPECT_EQ(onDisk, overrides.document());
}

} // namespace nioc::terminus
