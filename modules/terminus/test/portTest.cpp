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

fs::path testDataDir()
{
  return fs::path{"data"};
}

fs::path config()
{
  return testDataDir() / "testConfig.json";
}

fs::path configOverride()
{
  return testDataDir() / "testConfigOverride.json";
}

fs::path malformedConfig()
{
  return testDataDir() / "malformedConfig.json";
}

fs::path resource()
{
  return testDataDir() / "testResource.bin";
}

/// Shares testResource.bin's basename from a different directory to exercise the collision guard.
fs::path resourceDuplicate()
{
  return testDataDir() / "duplicate" / "testResource.bin";
}

/// Recording root for these tests; Port creates it on demand and names a unique recording under it.
fs::path logRoot()
{
  return fs::temp_directory_path() / "niocLogs";
}

std::string sampleCommandLine()
{
  return "myRobot --config /etc/foo.json";
}

/// Setup that builds no routines; these tests exercise the Port's recording duties directly.
void emptySetup(Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}

} // namespace

TEST(PortTest, constructionCreatesRecordingDirectory)
{
  const auto workingDir = [&]
  {
    auto port = Port{logRoot(), {config()}, {}, true, sampleCommandLine(), emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(recordingDir));
    EXPECT_EQ(recordingDir.parent_path(), logRoot());
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
      logRoot(),
      {config(), configOverride()},
      {},
      true,
      "",
      emptySetup
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
    auto port = Port{logRoot(), {config()}, {}, true, sampleCommandLine(), emptySetup};
    const auto recordingDir = port.workingDir();
    port.addResource(resource());

    EXPECT_TRUE(fs::is_regular_file(recordingDir / "testResource.bin"));
    return recordingDir;
  }();

  const auto meta = nlohmann::json::parse(std::ifstream(workingDir / "metadata.json"));
  EXPECT_EQ(meta.at("cmdline").get<std::string>(), sampleCommandLine());
  EXPECT_EQ(meta.at("resources").at(resource().string()).get<std::string>(), "testResource.bin");
}

TEST(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  auto port = Port{logRoot(), {config()}, {}, true, "", emptySetup};
  port.addResource(resource());

  const auto acquired = port.acquireResource(resource());
  EXPECT_EQ(acquired, port.workingDir() / "testResource.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST(PortTest, acquireResourceRejectsUnaddedResource)
{
  auto port = Port{logRoot(), {config()}, {}, true, "", emptySetup};
  EXPECT_THROW((void)port.acquireResource(testDataDir() / "never.bin"), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsBasenameCollision)
{
  auto port = Port{logRoot(), {config()}, {}, true, "", emptySetup};
  port.addResource(resource());
  EXPECT_THROW(port.addResource(resourceDuplicate()), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsMissingFile)
{
  auto port = Port{logRoot(), {config()}, {}, true, "", emptySetup};
  EXPECT_THROW(port.addResource(testDataDir() / "doesNotExist"), std::invalid_argument);
}

TEST(PortTest, constructionCreatesMissingLogRoot)
{
  const auto absentRoot = fs::temp_directory_path() / "niocPortTestAbsentRoot";
  fs::remove_all(absentRoot);

  auto port = Port{absentRoot, {}, {}, true, "", emptySetup};

  EXPECT_TRUE(fs::is_directory(absentRoot));
  EXPECT_EQ(port.workingDir().parent_path(), absentRoot);
}

TEST(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  const auto workingDir = [&]
  {
    auto port = Port{logRoot(), {config()}, {}, false, "", emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    return recordingDir;
  }();
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "metadata.json"));
}

TEST(PortTest, constructionAddsListedResources)
{
  auto port = Port{logRoot(), {config()}, {resource()}, true, "", emptySetup};
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
}

TEST(PortTest, constructorFromVariablesMapReadsOptions)
{
  // Build an argv that exercises every Port option, mirroring a real command line.
  const auto rootArg = logRoot().string();
  const auto configArg = config().string();
  const auto overrideArg = configOverride().string();
  const auto resourceArg = resource().string();
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

  auto port = Port{variableMap, emptySetup};

  EXPECT_EQ(port.workingDir().parent_path(), logRoot());
  EXPECT_EQ(port.config().at("robot").get<std::string>(), "atlas");
  EXPECT_EQ(port.config().at("sensors").at("lidarChannels").get<int>(), 32);
  EXPECT_EQ(port.config().at("mode").get<std::string>(), "override"); // later config wins
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false
}

TEST(PortTest, constructionRejectsUnreadableConfig)
{
  EXPECT_THROW(
      (Port{logRoot(), {testDataDir() / "doesNotExist.json"}, {}, true, "", emptySetup}),
      std::runtime_error);
}

TEST(PortTest, constructionRejectsMalformedConfig)
{
  EXPECT_THROW(
      (Port{logRoot(), {malformedConfig()}, {}, true, "", emptySetup}),
      nlohmann::json::parse_error);
}

} // namespace nioc::terminus
