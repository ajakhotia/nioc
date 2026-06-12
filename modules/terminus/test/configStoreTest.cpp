////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/schema.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <kj/exception.h>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/configStore.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// Writes @p text to a fresh file under the test's temp directory and returns its path.
fs::path writeTempConfig(const fs::path& filename, const std::string& text)
{
  const auto dir = fs::temp_directory_path() / "niocConfigStoreTest";
  fs::create_directories(dir);

  const auto path = dir / filename;
  std::ofstream(path) << text;
  return path;
}

} // namespace

TEST(ConfigStoreTest, emptyStoreYieldsSchemaDefaults)
{
  const auto store = ConfigStore{{}, {}, capnp::Schema::from<TestConfig>()};
  const auto config = store.get<TestConfig>();

  EXPECT_EQ(config.getCount(), 7U);
  EXPECT_TRUE(config.getEnabled());
  EXPECT_EQ(config.getLeaf().getValue(), 11); // the leaf's struct-literal default
  EXPECT_EQ(std::string{config.getLeaf().getTag().cStr()}, "lit");
}

TEST(ConfigStoreTest, filesMergeLeftToRight)
{
  const auto base = writeTempConfig("base.json", R"({"name": "base", "count": 1})");
  const auto overlay = writeTempConfig("overlay.json", R"({"count": 2})");

  const auto store = ConfigStore{
      {base, overlay},
      {},
      capnp::Schema::from<TestConfig>()
  };
  const auto config = store.get<TestConfig>();

  EXPECT_EQ(std::string{config.getName().cStr()}, "base");
  EXPECT_EQ(config.getCount(), 2U);
}

TEST(ConfigStoreTest, overridesApplyAfterFilesInOrder)
{
  const auto base = writeTempConfig("ordered.json", R"({"count": 1})");
  const auto store = ConfigStore{
      {base},
      {"count=2", "count=3"},
      capnp::Schema::from<TestConfig>()
  };
  EXPECT_EQ(store.get<TestConfig>().getCount(), 3U);
}

TEST(ConfigStoreTest, overrideValuesParseAsJson)
{
  const auto store = ConfigStore{
      {},
      {"name=hello", "count=42", "enabled=false", "gains=[1.5, 2.5]", "leaf.value=9"},
      capnp::Schema::from<TestConfig>()
  };
  const auto config = store.get<TestConfig>();

  EXPECT_EQ(std::string{config.getName().cStr()}, "hello");
  EXPECT_EQ(config.getCount(), 42U);
  EXPECT_FALSE(config.getEnabled());
  ASSERT_EQ(config.getGains().size(), 2U);
  EXPECT_DOUBLE_EQ(config.getGains()[0], 1.5);
  EXPECT_DOUBLE_EQ(config.getGains()[1], 2.5);
  EXPECT_EQ(config.getLeaf().getValue(), 9);
}

TEST(ConfigStoreTest, nullOverrideDeletesKey)
{
  // A raw (schema-less) store records the deletion itself; the revert-to-default behavior of the
  // schema pipeline is covered by schemaConstructionResolvesNullOverrideToExplicitDefault.
  const auto base = writeTempConfig("withName.json", R"({"name": "configured"})");
  const auto store = ConfigStore{{base}, {"name=null"}};

  const auto outPath = fs::temp_directory_path() / "niocConfigStoreTest" / "nullDeleted.json";
  store.writeTo(outPath);
  EXPECT_FALSE(nlohmann::json::parse(std::ifstream(outPath)).contains("name"));
}

TEST(ConfigStoreTest, unknownKeyFailsConstruction)
{
  EXPECT_THROW((ConfigStore{{}, {"bogus=1"}, capnp::Schema::from<TestConfig>()}), kj::Exception);
}

TEST(ConfigStoreTest, schemalessStoreRefusesTypedAccess)
{
  const auto store = ConfigStore{{}, {}};
  EXPECT_THROW((void)store.get<TestConfig>(), std::logic_error);
}

TEST(ConfigStoreTest, malformedOverrideThrows)
{
  EXPECT_THROW((ConfigStore{{}, {"noEqualsSign"}}), std::invalid_argument);
}

TEST(ConfigStoreTest, schemaConstructionMaterializesEveryDefault)
{
  const auto store = ConfigStore{{}, {}, capnp::Schema::from<TestConfig>()};

  const auto outPath = fs::temp_directory_path() / "niocConfigStoreTest" / "materialized.json";
  store.writeTo(outPath);
  const auto onDisk = nlohmann::json::parse(std::ifstream(outPath));

  // Every parameter is recorded explicitly, struct-literal and field defaults alike. JsonCodec
  // renders 64-bit integers as quoted strings to dodge json's double-precision limit.
  EXPECT_EQ(onDisk.at("count").get<int>(), 7);
  EXPECT_EQ(onDisk.at("leaf").at("value").get<std::string>(), "11");
  EXPECT_EQ(onDisk.at("leaf").at("tag").get<std::string>(), "lit");
}

TEST(ConfigStoreTest, schemaConstructionKeepsLiteralsThroughPartialOverrides)
{
  // Patching one leaf field must not reset its siblings: the struct-literal default survives
  // because it was materialized into the tree before the override landed.
  const auto config =
      ConfigStore{{}, {"leaf.tag=patched"}, capnp::Schema::from<TestConfig>()}.get<TestConfig>();
  EXPECT_EQ(std::string{config.getLeaf().getTag().cStr()}, "patched");
  EXPECT_EQ(config.getLeaf().getValue(), 11);
}

TEST(ConfigStoreTest, schemaConstructionResolvesNullOverrideToExplicitDefault)
{
  const auto store = ConfigStore{
      {},
      {"count=1", "count=null"},
      capnp::Schema::from<TestConfig>()
  };
  EXPECT_EQ(store.get<TestConfig>().getCount(), 7U);

  // The canonical recording shows the default the null reverted to, not a missing key.
  const auto outPath = fs::temp_directory_path() / "niocConfigStoreTest" / "nullReverted.json";
  store.writeTo(outPath);
  const auto onDisk = nlohmann::json::parse(std::ifstream(outPath));
  EXPECT_EQ(onDisk.at("count").get<int>(), 7);
}

TEST(ConfigStoreTest, writeToRecordsMergedTree)
{
  const auto base = writeTempConfig("recorded.json", R"({"count": 5})");
  const auto store = ConfigStore{{base}, {"name=run1"}};

  const auto outPath = fs::temp_directory_path() / "niocConfigStoreTest" / "merged.json";
  store.writeTo(outPath);

  const auto onDisk = nlohmann::json::parse(std::ifstream(outPath));
  EXPECT_EQ(onDisk.at("count").get<int>(), 5);
  EXPECT_EQ(onDisk.at("name").get<std::string>(), "run1");
}

} // namespace nioc::terminus
