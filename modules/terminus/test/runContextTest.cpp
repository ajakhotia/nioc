////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

boost::program_options::variables_map parse(std::vector<const char*> arguments)
{
  arguments.insert(arguments.begin(), "runContextTest");

  return parseCommandLine(
      static_cast<int>(arguments.size()),
      arguments.data(),
      RunContext::cliOptions());
}

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class RunContextTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  RunContextTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(RunContextTest, onlineByDefault)
{
  const auto context = RunContext{parse({})};
  EXPECT_FALSE(context.playback());
}

TEST_F(RunContextTest, mintsAndCreatesWorkingDirUnderLogRoot)
{
  const auto root = path();
  const auto rootArg = root.string();
  const auto context = RunContext{parse({"--log-root", rootArg.c_str()})};

  EXPECT_EQ(context.workingDir().parent_path(), root);
  EXPECT_TRUE(fs::is_directory(context.workingDir()));
}

TEST_F(RunContextTest, readsRecordChronicleAndCommandLine)
{
  const auto root = path();
  const auto rootArg = root.string();
  const auto context = RunContext{
      parse({"--log-root", rootArg.c_str(), "--record-chronicle", "false"})};

  EXPECT_FALSE(context.recordChronicle());
  EXPECT_FALSE(context.commandLine().empty());
}

TEST_F(RunContextTest, constructionEstablishesTheRunOnDisk)
{
  // Constructing a context creates the working directory and writes both the assembled overlay and
  // the manifest into it, before any Port exists.
  const auto context = RunContext{
      path(),
      {},
      true,
      "myRobot --run",
      {},
      {},
      {"routines.drivers.hiroHills.miningTimeMs=5"}};

  const auto& workingDir = context.workingDir();
  ASSERT_TRUE(fs::is_regular_file(workingDir / "configOverlay.json"));
  ASSERT_TRUE(fs::is_regular_file(workingDir / "manifest.json"));

  const auto manifest = nlohmann::json::parse(std::ifstream(workingDir / "manifest.json"));
  EXPECT_EQ(manifest.at("cmdline").get<std::string>(), "myRobot --run");
  EXPECT_EQ(manifest.at("mode").get<std::string>(), "online");

  // The persisted overlay carries the assembled overrides, and the context exposes the same
  // document through its ConfigOverlay.
  const auto onDisk = nlohmann::json::parse(std::ifstream(workingDir / "configOverlay.json"));
  EXPECT_EQ(onDisk, context.configOverlay().document());
  EXPECT_EQ(onDisk.at(nlohmann::json::json_pointer{"/routines/drivers/hiroHills/miningTimeMs"}), 5);
}

} // namespace nioc::terminus
