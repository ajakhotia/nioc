////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/serialize.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/containers/mmapConstArray.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <nioc/terminus/config.hpp>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nlohmann/json.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

fs::path testDirectory()
{
  return fs::temp_directory_path() / "niocConfigTest";
}

nlohmann::json readArtifact(const std::string& name, const std::string& extension)
{
  return nlohmann::json::parse(std::ifstream(testDirectory() / (name + extension)));
}

} // namespace

TEST(ConfigTest, defaultsAloneYieldSchemaDefaults)
{
  const auto config = Config<TestConfig>{nlohmann::json::object(), testDirectory(), "defaults"};
  const auto reader = config.reader();

  EXPECT_EQ(reader.getCount(), 7U);
  EXPECT_TRUE(reader.getEnabled());
  EXPECT_EQ(reader.getLeaf().getValue(), 11); // the leaf's struct-literal default
  EXPECT_EQ(std::string{reader.getLeaf().getTag().cStr()}, "lit");
}

TEST(ConfigTest, overridesMergeOntoDefaults)
{
  // Patching one leaf field must not reset its siblings: the struct-literal default survives
  // because the overrides merge onto the fully materialized default tree.
  const auto overrides = nlohmann::json{{"count", 2}, {"leaf", {{"tag", "patched"}}}};
  const auto config = Config<TestConfig>{overrides, testDirectory(), "merged"};
  const auto reader = config.reader();

  EXPECT_EQ(reader.getCount(), 2U);
  EXPECT_EQ(reader.getLeaf().getValue(), 11);
  EXPECT_EQ(std::string{reader.getLeaf().getTag().cStr()}, "patched");
}

TEST(ConfigTest, nullOverrideRevertsToSchemaDefault)
{
  const auto overrides = nlohmann::json{{"count", nullptr}};
  const auto config = Config<TestConfig>{overrides, testDirectory(), "nullReverted"};

  EXPECT_EQ(config.reader().getCount(), 7U);
}

TEST(ConfigTest, offSchemaKeysAreToleratedButNotRecorded)
{
  // Overrides may carry fields outside the current schema (e.g. written by a newer build). They
  // are tolerated at decode time rather than rejected, but the effective record is the schema
  // projection, so an off-schema field never appears in it.
  const auto overrides = nlohmann::json{{"count", 5}, {"futureField", 42}};
  const auto config = Config<TestConfig>{overrides, testDirectory(), "offSchema"};

  EXPECT_EQ(config.reader().getCount(), 5U);

  const auto onDisk = readArtifact("offSchema", ".json");
  EXPECT_EQ(onDisk.at("count").get<int>(), 5);
  EXPECT_FALSE(onDisk.contains("futureField"));
}

TEST(ConfigTest, effectiveJsonMaterializesEveryDefault)
{
  const auto config = Config<TestConfig>{nlohmann::json::object(), testDirectory(), "materialized"};

  const auto onDisk = readArtifact("materialized", ".json");

  // Every parameter is recorded explicitly, struct-literal and field defaults alike. JsonCodec
  // renders 64-bit integers as quoted strings to dodge json's double-precision limit.
  EXPECT_EQ(onDisk.at("count").get<int>(), 7);
  EXPECT_EQ(onDisk.at("leaf").at("value").get<std::string>(), "11");
  EXPECT_EQ(onDisk.at("leaf").at("tag").get<std::string>(), "lit");
}

TEST(ConfigTest, binaryArtifactLoadsIndependently)
{
  const auto overrides = nlohmann::json{{"name", "mapped"}, {"gains", {1.5, 2.5}}};
  const auto config = Config<TestConfig>{overrides, testDirectory(), "standalone"};

  // Reopen the artifact through a fresh mapping and reader, independent of the Config that wrote
  // it.
  const auto mapping = containers::MmapConstArray<std::byte>{testDirectory() / "standalone.bin"};
  auto message = capnp::FlatArrayMessageReader{
      asWords(std::span<const std::byte>{mapping.data(), mapping.size()})};
  const auto reader = message.getRoot<TestConfig>();

  EXPECT_EQ(std::string{reader.getName().cStr()}, "mapped");
  EXPECT_EQ(reader.getCount(), 7U);
  ASSERT_EQ(reader.getGains().size(), 2U);
  // Materialize the Cap'n Proto List reader into a vector so the elements can be indexed via the
  // checked .at() accessor (the reader itself offers only operator[]).
  auto gains = std::vector<double>{};
  for(const auto gain: reader.getGains())
  {
    gains.push_back(gain);
  }
  EXPECT_DOUBLE_EQ(gains.at(0), 1.5);
  EXPECT_DOUBLE_EQ(gains.at(1), 2.5);
}

TEST(ConfigTest, nonObjectOverridesThrow)
{
  EXPECT_THROW(
      (Config<TestConfig>{nlohmann::json(42), testDirectory(), "nonObject"}),
      std::invalid_argument);
}

TEST(ConfigTest, missingDirectoriesAreCreated)
{
  const auto directory = testDirectory() / "nested" / "deep";
  const auto config = Config<TestConfig>{nlohmann::json::object(), directory, "cfg"};

  EXPECT_TRUE(fs::exists(directory / "cfg.json"));
  EXPECT_TRUE(fs::exists(directory / "cfg.bin"));
}

TEST(ConfigTest, movedConfigKeepsReading)
{
  auto source = Config<TestConfig>{nlohmann::json{{"name", "mover"}}, testDirectory(), "moved"};

  const auto config = Config<TestConfig>{std::move(source)};

  EXPECT_EQ(std::string{config.reader().getName().cStr()}, "mover");
  EXPECT_EQ(config.reader().getCount(), 7U);
}

} // namespace nioc::terminus
