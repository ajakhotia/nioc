////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/programOption.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// Parses @p arguments against the manifest's options, as a real main would.
boost::program_options::variables_map parse(std::vector<const char*> arguments)
{
  arguments.insert(arguments.begin(), "manifestTest");

  auto options = Manifest::cliOptions();
  return parseCommandLine(static_cast<int>(arguments.size()), arguments.data(), options);
}

/// Creates a minimal recording — a directory holding the given config.json — and returns its path.
fs::path makeRecording(const std::string& name, const std::string& configText)
{
  const auto dir = fs::temp_directory_path() / "niocManifestTest" / name;
  fs::create_directories(dir);
  std::ofstream(dir / "config.json") << configText;
  return dir;
}

} // namespace

TEST(ManifestTest, onlineByDefault)
{
  const auto manifest = Manifest{parse({}), capnp::Schema::from<TestConfig>()};
  EXPECT_FALSE(manifest.mContext.playback());
}

TEST(ManifestTest, contextReadsCliOptions)
{
  const auto root = fs::temp_directory_path() / "niocManifestTestRoot";
  const auto rootArg = root.string();
  const auto manifest = Manifest{
      parse({"--log-root", rootArg.c_str(), "--record-chronicle", "false"}),
      capnp::Schema::from<TestConfig>()};

  EXPECT_EQ(manifest.mContext.logRoot(), root);
  EXPECT_FALSE(manifest.mContext.recordChronicle());
  EXPECT_FALSE(manifest.mContext.commandLine().empty());
}

TEST(ManifestTest, playbackLayersRecordedConfigBeneathOverrides)
{
  const auto recording = makeRecording("replayed", R"({"name": "recorded", "count": 5})");
  const auto recordingArg = recording.string();

  const auto manifest = Manifest{
      parse({"--playback", recordingArg.c_str(), "--config-override", "name=cli"}),
      capnp::Schema::from<TestConfig>()};

  EXPECT_TRUE(manifest.mContext.playback());
  EXPECT_EQ(manifest.mContext.inputLog(), recording);

  // The recording pins count; the override outranks the recording for name.
  const auto config = manifest.mConfigStore.get<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
  EXPECT_EQ(std::string{config.getName().cStr()}, "cli");
}

TEST(ManifestTest, playbackRejectsNonRecording)
{
  const auto notARecording = fs::temp_directory_path() / "niocManifestTest" / "doesNotExist";
  const auto pathArg = notARecording.string();
  EXPECT_THROW(
      (Manifest{parse({"--playback", pathArg.c_str()}), capnp::Schema::from<TestConfig>()}),
      std::invalid_argument);
}

} // namespace nioc::terminus
