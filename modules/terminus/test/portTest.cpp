////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// Committed fixtures, staged into this test's working directory by CMake (see CMakeLists.txt).
const auto kTestDataDir = fs::path{"data"};
const auto kConfig = kTestDataDir / "testConfig.json";
const auto kConfigOverride = kTestDataDir / "testConfigOverride.json";
const auto kMalformedConfig = kTestDataDir / "malformedConfig.json";
const auto kResource = kTestDataDir / "testResource.bin";

/// Shares testResource.bin's basename from a different directory to exercise the collision guard.
const auto kResourceDuplicate = kTestDataDir / "duplicate" / "testResource.bin";

/// Recording root for these tests; Port creates it on demand and names a unique recording under it.
const auto kLogRoot = fs::temp_directory_path() / "niocLogs";

const auto kSampleCommandLine = std::string{"myRobot --config /etc/foo.json"};

} // namespace

TEST(PortTest, constructionCreatesRecordingDirectory)
{
  const auto workingDir = [&]
  {
    auto port = Port{kLogRoot, {kConfig}, {}, true, kSampleCommandLine};
    const auto recordingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(recordingDir));
    EXPECT_EQ(recordingDir.parent_path(), kLogRoot);
    EXPECT_TRUE(fs::is_directory(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "console.log"));
    return recordingDir;
  }();
  EXPECT_TRUE(fs::is_regular_file(workingDir / "metadata.json"));
}

TEST(PortTest, configMergesPathsLeftToRight)
{
  auto port = Port{
      kLogRoot,
      {kConfig, kConfigOverride},
      {},
      true,
      ""
  };
  const auto& cfg = port.config();

  EXPECT_EQ(cfg.at("robot").get<std::string>(), "atlas");          // base only
  EXPECT_EQ(cfg.at("controlRateHz").get<int>(), 200);              // base only
  EXPECT_TRUE(cfg.at("sensors").at("imu").get<bool>());            // base only
  EXPECT_EQ(cfg.at("sensors").at("lidarChannels").get<int>(), 32); // override wins
  EXPECT_TRUE(cfg.at("sensors").at("camera").get<bool>());         // override only
  EXPECT_EQ(cfg.at("mode").get<std::string>(), "override");        // override wins

  // The merged config is written verbatim to <dir>/config.json.
  const auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));
  EXPECT_EQ(onDisk, cfg);
}

TEST(PortTest, metadataIncludesCmdlineAndResources)
{
  const auto workingDir = [&]
  {
    auto port = Port{kLogRoot, {kConfig}, {}, true, kSampleCommandLine};
    const auto recordingDir = port.workingDir();
    port.addResource(kResource);

    EXPECT_TRUE(fs::is_regular_file(recordingDir / "testResource.bin"));
    return recordingDir;
  }();

  const auto meta = nlohmann::json::parse(std::ifstream(workingDir / "metadata.json"));
  EXPECT_EQ(meta.at("cmdline").get<std::string>(), kSampleCommandLine);
  EXPECT_EQ(meta.at("resources").at(kResource.string()).get<std::string>(), "testResource.bin");
}

TEST(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  auto port = Port{kLogRoot, {kConfig}, {}, true, ""};
  port.addResource(kResource);

  const auto acquired = port.acquireResource(kResource);
  EXPECT_EQ(acquired, port.workingDir() / "testResource.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST(PortTest, acquireResourceRejectsUnaddedResource)
{
  auto port = Port{kLogRoot, {kConfig}, {}, true, ""};
  EXPECT_THROW((void)port.acquireResource(kTestDataDir / "never.bin"), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsBasenameCollision)
{
  auto port = Port{kLogRoot, {kConfig}, {}, true, ""};
  port.addResource(kResource);
  EXPECT_THROW(port.addResource(kResourceDuplicate), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsMissingFile)
{
  auto port = Port{kLogRoot, {kConfig}, {}, true, ""};
  EXPECT_THROW(port.addResource(kTestDataDir / "doesNotExist"), std::invalid_argument);
}

TEST(PortTest, constructionCreatesMissingLogRoot)
{
  const auto absentRoot = fs::temp_directory_path() / "niocPortTestAbsentRoot";
  fs::remove_all(absentRoot);

  auto port = Port{absentRoot, {}, {}, true, ""};

  EXPECT_TRUE(fs::is_directory(absentRoot));
  EXPECT_EQ(port.workingDir().parent_path(), absentRoot);
}

TEST(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  const auto workingDir = [&]
  {
    auto port = Port{kLogRoot, {kConfig}, {}, false, ""};
    const auto recordingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    return recordingDir;
  }();
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "metadata.json"));
}

TEST(PortTest, constructionAddsListedResources)
{
  auto port = Port{kLogRoot, {kConfig}, {kResource}, true, ""};
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
}

TEST(PortTest, constructorFromVariablesMapReadsOptions)
{
  // Build an argv that exercises every Port option, mirroring a real command line.
  const auto rootArg = kLogRoot.string();
  const auto configArg = kConfig.string();
  const auto overrideArg = kConfigOverride.string();
  const auto resourceArg = kResource.string();
  const auto argv = std::array<const char*, 11>{
      "myRobot",
      "--log-root",
      rootArg.c_str(),
      "--append-config",
      configArg.c_str(),
      "--append-config",
      overrideArg.c_str(),
      "--append-resource",
      resourceArg.c_str(),
      "--record-chronicle",
      "false"};
  constexpr auto argc = static_cast<int>(argv.size());

  auto options = programOptions("myRobot");
  const auto variableMap = parseCommandLine(argc, argv.data(), options);

  // parseCommandLine injects the verbatim command line for the constructor to record.
  EXPECT_TRUE(variableMap.contains("commandLine"));

  auto port = Port{variableMap};

  EXPECT_EQ(port.workingDir().parent_path(), kLogRoot);
  EXPECT_EQ(port.config().at("robot").get<std::string>(), "atlas");
  EXPECT_EQ(port.config().at("sensors").at("lidarChannels").get<int>(), 32);
  EXPECT_EQ(port.config().at("mode").get<std::string>(), "override"); // later config wins
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false
}

TEST(PortTest, constructionRejectsUnreadableConfig)
{
  EXPECT_THROW(
      (Port{kLogRoot, {kTestDataDir / "doesNotExist.json"}, {}, true, ""}),
      std::runtime_error);
}

TEST(PortTest, constructionRejectsMalformedConfig)
{
  EXPECT_THROW((Port{kLogRoot, {kMalformedConfig}, {}, true, ""}), nlohmann::json::parse_error);
}

} // namespace nioc::terminus
