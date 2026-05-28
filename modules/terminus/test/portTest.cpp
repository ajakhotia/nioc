////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <boost/program_options.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nlohmann/json.hpp>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// Create a fresh empty directory at a deterministic path under the system temp directory.
/// Any prior contents are wiped.
fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-portTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

void writeFile(const fs::path& path, std::string_view contents)
{
  fs::create_directories(path.parent_path());
  auto out = std::ofstream(path);
  out << contents;
}

const auto kSampleCommandLine = std::string{ "myRobot --config /etc/foo.json" };

} // namespace

TEST(PortTest, constructionCreatesRecordingDirectory)
{
  const auto root = makeFreshEmptyDir("ctor-creates");
  const auto configPath = root / "in" / "base.json";
  writeFile(configPath, R"({"hello": "world"})");

  fs::path workingDir;
  {
    auto port = Port{ root, { configPath }, {}, true, kSampleCommandLine };
    workingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(workingDir));
    EXPECT_EQ(workingDir.parent_path(), root);
    EXPECT_TRUE(fs::is_directory(workingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(workingDir / "config.json"));
    EXPECT_TRUE(fs::is_regular_file(workingDir / "console.log"));
  }
  EXPECT_TRUE(fs::is_regular_file(workingDir / "metadata.json"));
}

TEST(PortTest, configMergesPathsLeftToRight)
{
  const auto root = makeFreshEmptyDir("config-merge");
  const auto base = root / "in" / "base.json";
  const auto override_ = root / "in" / "override.json";
  writeFile(base, R"({"a": 1, "b": {"x": 10, "y": 20}, "c": "base"})");
  writeFile(override_, R"({"b": {"y": 99, "z": 30},        "c": "override"})");

  auto port = Port{
    root,
    { base, override_ },
    {},
    true,
    ""
  };
  const auto& cfg = port.config();

  EXPECT_EQ(cfg.at("a").get<int>(), 1);
  EXPECT_EQ(cfg.at("b").at("x").get<int>(), 10);
  EXPECT_EQ(cfg.at("b").at("y").get<int>(), 99);
  EXPECT_EQ(cfg.at("b").at("z").get<int>(), 30);
  EXPECT_EQ(cfg.at("c").get<std::string>(), "override");

  // The merged config is written verbatim to <dir>/config.json.
  auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));
  EXPECT_EQ(onDisk, cfg);
}

TEST(PortTest, metadataIncludesCmdlineAndResources)
{
  const auto root = makeFreshEmptyDir("metadata");
  const auto configPath = root / "in" / "c.json";
  writeFile(configPath, R"({})");
  const auto resA = root / "external" / "urdf" / "robot.urdf";
  const auto resB = root / "external" / "cal" / "imu.yaml";
  writeFile(resA, "URDF-CONTENTS");
  writeFile(resB, "CAL-CONTENTS");

  fs::path workingDir;
  {
    auto port = Port{ root, { configPath }, {}, true, kSampleCommandLine };
    workingDir = port.workingDir();
    port.addResource(resA);
    port.addResource(resB);

    EXPECT_TRUE(fs::is_regular_file(workingDir / "robot.urdf"));
    EXPECT_TRUE(fs::is_regular_file(workingDir / "imu.yaml"));
  }

  const auto meta = nlohmann::json::parse(std::ifstream(workingDir / "metadata.json"));
  EXPECT_EQ(meta.at("cmdline").get<std::string>(), kSampleCommandLine);
  EXPECT_EQ(meta.at("resources").at(resA.string()).get<std::string>(), "robot.urdf");
  EXPECT_EQ(meta.at("resources").at(resB.string()).get<std::string>(), "imu.yaml");
}

TEST(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  const auto root = makeFreshEmptyDir("acquire");
  writeFile(root / "in" / "c.json", "{}");
  const auto resource = root / "external" / "thing.bin";
  writeFile(resource, "bytes");

  auto port = Port{ root, { root / "in" / "c.json" }, {}, true, "" };
  port.addResource(resource);

  const auto acquired = port.acquireResource(resource);
  EXPECT_EQ(acquired, port.workingDir() / "thing.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST(PortTest, acquireResourceRejectsUnaddedResource)
{
  const auto root = makeFreshEmptyDir("acquire-unknown");
  writeFile(root / "in" / "c.json", "{}");

  auto port = Port{ root, { root / "in" / "c.json" }, {}, true, "" };
  EXPECT_THROW((void)port.acquireResource(root / "never.bin"), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsBasenameCollision)
{
  const auto root = makeFreshEmptyDir("collision");
  writeFile(root / "in" / "c.json", "{}");
  const auto a = root / "ext" / "a" / "thing.bin";
  const auto b = root / "ext" / "b" / "thing.bin";
  writeFile(a, "A");
  writeFile(b, "B");

  auto port = Port{ root, { root / "in" / "c.json" }, {}, true, "" };
  port.addResource(a);
  EXPECT_THROW(port.addResource(b), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsMissingFile)
{
  const auto root = makeFreshEmptyDir("missing-resource");
  writeFile(root / "in" / "c.json", "{}");

  auto port = Port{ root, { root / "in" / "c.json" }, {}, true, "" };
  EXPECT_THROW(port.addResource(root / "doesNotExist"), std::invalid_argument);
}

TEST(PortTest, constructionCreatesMissingLogRoot)
{
  const auto root = fs::temp_directory_path() / "nioc-portTest" / "absent-root";
  fs::remove_all(root);

  auto port = Port{ root, {}, {}, true, "" };

  EXPECT_TRUE(fs::is_directory(root));
  EXPECT_EQ(port.workingDir().parent_path(), root);
}

TEST(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  const auto root = makeFreshEmptyDir("no-chronicle");
  writeFile(root / "in" / "c.json", "{}");

  fs::path workingDir;
  {
    auto port = Port{ root, { root / "in" / "c.json" }, {}, false, "" };
    workingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(workingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(workingDir / "config.json"));
  }
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "metadata.json"));
}

TEST(PortTest, constructionAddsListedResources)
{
  const auto root = makeFreshEmptyDir("ctor-resources");
  writeFile(root / "in" / "c.json", "{}");
  const auto resource = root / "ext" / "robot.urdf";
  writeFile(resource, "URDF");

  auto port = Port{ root, { root / "in" / "c.json" }, { resource }, true, "" };

  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "robot.urdf"));
}

TEST(PortTest, constructorFromVariablesMapReadsOptions)
{
  const auto root = makeFreshEmptyDir("vm-ctor");
  const auto cfgA = root / "in" / "a.json";
  const auto cfgB = root / "in" / "b.json";
  writeFile(cfgA, R"({"a": 1, "shared": "A"})");
  writeFile(cfgB, R"({"b": 2, "shared": "B"})");
  const auto resource = root / "ext" / "thing.bin";
  writeFile(resource, "bytes");

  // Build an argv that exercises every Port option, mirroring a real command line.
  const auto rootArg = root.string();
  const auto cfgAArg = cfgA.string();
  const auto cfgBArg = cfgB.string();
  const auto resArg = resource.string();
  const auto argv = std::array<const char*, 11>{
    "myRobot",       "--log-root",         rootArg.c_str(), "--append-config",
    cfgAArg.c_str(), "--append-config",    cfgBArg.c_str(), "--append-resource",
    resArg.c_str(),  "--record-chronicle", "false"
  };
  constexpr auto argc = static_cast<int>(argv.size());

  auto options = programOptions("myRobot");
  const auto variableMap = parseCommandLine(argc, argv.data(), options);

  // parseCommandLine injects the verbatim command line for the constructor to record.
  EXPECT_TRUE(variableMap.contains("commandLine"));

  auto port = Port{ variableMap };

  EXPECT_EQ(port.workingDir().parent_path(), root);
  EXPECT_EQ(port.config().at("a").get<int>(), 1);
  EXPECT_EQ(port.config().at("b").get<int>(), 2);
  EXPECT_EQ(port.config().at("shared").get<std::string>(), "B"); // later config wins
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "thing.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false
}

TEST(PortTest, constructionRejectsUnreadableConfig)
{
  const auto root = makeFreshEmptyDir("bad-config");
  EXPECT_THROW((Port{ root, { root / "doesNotExist.json" }, {}, true, "" }), std::runtime_error);
}

TEST(PortTest, constructionRejectsMalformedConfig)
{
  const auto root = makeFreshEmptyDir("malformed-config");
  const auto bad = root / "in" / "bad.json";
  writeFile(bad, "{ not json");
  EXPECT_THROW((Port{ root, { bad }, {}, true, "" }), nlohmann::json::parse_error);
}

} // namespace nioc::terminus
