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
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>

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
void emptySetup(Port& /*unused*/, Port::Drivers& /*unused*/, Port::Components& /*unused*/, Port::Runners& /*unused*/) {}

/// A manifest recording under logRoot() with the standard test config and no extras.
Manifest testManifest(std::string commandLine = "")
{
  return Manifest{
      RunContext{logRoot(), {}, true, std::move(commandLine)},
      ConfigStore{{config()}, {}}
  };
}

} // namespace

TEST(PortTest, constructionCreatesRecordingDirectory)
{
  const auto workingDir = [&]
  {
    auto port = Port{testManifest(sampleCommandLine()), emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(recordingDir));
    EXPECT_EQ(recordingDir.parent_path(), logRoot());
    EXPECT_TRUE(fs::is_directory(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "manifest.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "console.log"));
    return recordingDir;
  }();
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST(PortTest, configMergesPathsLeftToRight)
{
  auto port = Port{
      Manifest{RunContext{logRoot(), {}, true, ""}, ConfigStore{{config(), configOverride()}, {}}},
      emptySetup
  };

  // The merged config is recorded to <dir>/config.json; verify the merge through the recording.
  const auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));

  EXPECT_EQ(onDisk.at("robot").get<std::string>(), "atlas");          // base only
  EXPECT_EQ(onDisk.at("controlRateHz").get<int>(), 200);              // base only
  EXPECT_TRUE(onDisk.at("sensors").at("imu").get<bool>());            // base only
  EXPECT_EQ(onDisk.at("sensors").at("lidarChannels").get<int>(), 32); // override wins
  EXPECT_TRUE(onDisk.at("sensors").at("camera").get<bool>());         // override only
  EXPECT_EQ(onDisk.at("mode").get<std::string>(), "override");        // override wins
}

TEST(PortTest, recordingCarriesManifestAndResources)
{
  const auto workingDir = [&]
  {
    auto port = Port{testManifest(sampleCommandLine()), emptySetup};
    const auto recordingDir = port.workingDir();
    port.addResource(resource());

    EXPECT_TRUE(fs::is_regular_file(recordingDir / "testResource.bin"));
    return recordingDir;
  }();

  const auto manifest = nlohmann::json::parse(std::ifstream(workingDir / "manifest.json"));
  EXPECT_EQ(manifest.at("cmdline").get<std::string>(), sampleCommandLine());
  EXPECT_EQ(manifest.at("mode").get<std::string>(), "online");

  // resources.json is rewritten at teardown, so it carries the resource added mid-run.
  const auto resources = nlohmann::json::parse(std::ifstream(workingDir / "resources.json"));
  EXPECT_EQ(resources.at(resource().string()).get<std::string>(), "testResource.bin");
}

TEST(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  auto port = Port{testManifest(), emptySetup};
  port.addResource(resource());

  const auto acquired = port.acquireResource(resource());
  EXPECT_EQ(acquired, port.workingDir() / "testResource.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST(PortTest, acquireResourceRejectsUnaddedResource)
{
  auto port = Port{testManifest(), emptySetup};
  EXPECT_THROW((void)port.acquireResource(testDataDir() / "never.bin"), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsBasenameCollision)
{
  auto port = Port{testManifest(), emptySetup};
  port.addResource(resource());
  EXPECT_THROW(port.addResource(resourceDuplicate()), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsMissingFile)
{
  auto port = Port{testManifest(), emptySetup};
  EXPECT_THROW(port.addResource(testDataDir() / "doesNotExist"), std::invalid_argument);
}

TEST(PortTest, constructionCreatesMissingLogRoot)
{
  const auto absentRoot = fs::temp_directory_path() / "niocPortTestAbsentRoot";
  fs::remove_all(absentRoot);

  auto port = Port{
      Manifest{RunContext{absentRoot, {}, true, ""}, ConfigStore{{}, {}}},
      emptySetup
  };

  EXPECT_TRUE(fs::is_directory(absentRoot));
  EXPECT_EQ(port.workingDir().parent_path(), absentRoot);
}

TEST(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  const auto workingDir = [&]
  {
    auto port = Port{
        Manifest{RunContext{logRoot(), {}, false, ""}, ConfigStore{{config()}, {}}},
        emptySetup
    };
    const auto& recordingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    return recordingDir;
  }();
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST(PortTest, constructionAddsListedResources)
{
  auto port = Port{
      Manifest{RunContext{logRoot(), {resource()}, true, ""}, ConfigStore{{config()}, {}}},
      emptySetup
  };
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
}

TEST(PortTest, constructionFromCommandLineReadsEveryOption)
{
  // Stage config files compatible with TestConfig, then build an argv that exercises every
  // manifest option, mirroring a real command line.
  const auto stagingDir = fs::temp_directory_path() / "niocPortTestCli";
  fs::create_directories(stagingDir);
  const auto base = stagingDir / "base.json";
  std::ofstream(base) << R"({"name": "base", "count": 1})";
  const auto overlay = stagingDir / "overlay.json";
  std::ofstream(overlay) << R"({"count": 2})";

  const auto rootArg = logRoot().string();
  const auto baseArg = base.string();
  const auto overlayArg = overlay.string();
  const auto resourceArg = resource().string();
  const auto argv = std::array<const char*, 13>{
      "myRobot",
      "--log-root",
      rootArg.c_str(),
      "--append-config",
      baseArg.c_str(),
      "--append-config",
      overlayArg.c_str(),
      "--config-override",
      "name=cli",
      "--append-resource",
      resourceArg.c_str(),
      "--record-chronicle",
      "false"};
  constexpr auto argc = static_cast<int>(argv.size());

  auto options = programOptions("myRobot");
  options.add(Manifest::cliOptions());
  const auto variableMap = parseCommandLine(argc, argv.data(), options);

  // parseCommandLine injects the verbatim command line for the Port to record.
  EXPECT_TRUE(variableMap.contains("commandLine"));

  auto port = Port{
      Manifest{variableMap, capnp::Schema::from<TestConfig>()},
      emptySetup
  };

  EXPECT_EQ(port.workingDir().parent_path(), logRoot());
  EXPECT_FALSE(port.runContext().playback());
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false

  // The recorded config.json carries the file merge with the cli override applied on top.
  const auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));
  EXPECT_EQ(onDisk.at("count").get<int>(), 2);            // later file wins
  EXPECT_EQ(onDisk.at("name").get<std::string>(), "cli"); // --config-override wins over files
}

TEST(PortTest, constructionRejectsUnreadableConfig)
{
  EXPECT_THROW(
      (Port{
          Manifest{
                   RunContext{logRoot(), {}, true, ""},
                   ConfigStore{{testDataDir() / "doesNotExist.json"}, {}}},
          emptySetup
  }),
      std::runtime_error);
}

TEST(PortTest, constructionRejectsMalformedConfig)
{
  EXPECT_THROW(
      (Port{
          Manifest{RunContext{logRoot(), {}, true, ""}, ConfigStore{{malformedConfig()}, {}}},
          emptySetup
  }),
      nlohmann::json::parse_error);
}

} // namespace nioc::terminus
