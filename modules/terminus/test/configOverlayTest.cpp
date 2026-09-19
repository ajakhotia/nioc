////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <nioc/terminus/configOverlay.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class ConfigOverlayTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  ConfigOverlayTest(): ScratchDirectory{unitTestDirectory()} {}

protected:
  [[nodiscard]] fs::path writeConfigFile(const fs::path& name, const std::string& text) const
  {
    const auto path = this->path() / name;
    fs::create_directories(path.parent_path());
    std::ofstream(path) << text;
    return path;
  }

  /// Stage a recording directory holding a `configOverlay.json`, as playback reads it.
  [[nodiscard]] fs::path makeRecording(const fs::path& name, const std::string& overlayText) const
  {
    const auto dir = path() / name;
    fs::create_directories(dir);
    std::ofstream(dir / "configOverlay.json") << overlayText;
    return dir;
  }
};

} // namespace

TEST_F(ConfigOverlayTest, layersFilesLeftToRightThenOverrides)
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

TEST_F(ConfigOverlayTest, overrideCreatesAnAbsentPath)
{
  const auto overrides = ConfigOverlay{{}, {}, {"routines.drivers.hiroHills.miningTimeMs=5"}};

  EXPECT_EQ(
      overrides.document().at(
          nlohmann::json::json_pointer{"/routines/drivers/hiroHills/miningTimeMs"}),
      5);
}

TEST_F(ConfigOverlayTest, playbackLayersRecordedOverlayBeneathThisRun)
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

TEST_F(ConfigOverlayTest, playbackRejectsNonRecording)
{
  EXPECT_THROW((ConfigOverlay{path() / "noSuchRecording", {}, {}}), std::invalid_argument);
}

TEST_F(ConfigOverlayTest, overridesLookupIsByNameAcrossSections)
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

TEST_F(ConfigOverlayTest, acquireOverridesReturnsEmptyForUnknownRoutine)
{
  const auto overlay = ConfigOverlay{{}, {}, {}};
  EXPECT_TRUE(overlay.acquireOverrides("neverConfigured").empty());
}

TEST_F(ConfigOverlayTest, rejectsNameInMoreThanOneSection)
{
  const auto config = writeConfigFile(
      "collision.json",
      R"({"routines": {
            "drivers": {"twin": {"miningTimeMs": 1}},
            "components": {"twin": {"brickPerRoad": 1}}}})");

  EXPECT_THROW((ConfigOverlay{{}, {config}, {}}), std::invalid_argument);
}

TEST_F(ConfigOverlayTest, writePersistsTheDocument)
{
  const auto overrides = ConfigOverlay{{}, {}, {"routines.drivers.hiroHills.miningTimeMs=4"}};
  const auto directory = path() / "persisted";
  fs::create_directories(directory);

  overrides.write(directory);

  const auto onDisk = nlohmann::json::parse(std::ifstream(directory / "configOverlay.json"));
  EXPECT_EQ(onDisk, overrides.document());
}

} // namespace nioc::terminus
