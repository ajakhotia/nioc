////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace nioc::terminus
{
namespace
{
namespace fs = std::filesystem;

fs::path testDirectory()
{
  const auto directory = fs::temp_directory_path() / "niocUtilsTest";
  fs::create_directories(directory);
  return directory;
}

TEST(UtilsTest, encodeAsJsonRendersSchemaDefaults)
{
  const auto defaults = encodeAsJson(capnp::Schema::from<TestConfig>());

  EXPECT_EQ(defaults.at("name").get<std::string>(), ""); // Text with no default -> empty
  EXPECT_EQ(defaults.at("count").get<int>(), 7);
  EXPECT_TRUE(defaults.at("enabled").get<bool>());
  EXPECT_TRUE(defaults.at("gains").is_array());
  EXPECT_TRUE(defaults.at("gains").empty());    // List with no default -> empty array
  EXPECT_TRUE(defaults.at("leaf").is_object()); // nested struct -> nested object
}

TEST(UtilsTest, encodeAsJsonSurfacesStructLiteralDefaultsAndQuotes64BitIntegers)
{
  const auto defaults = encodeAsJson(capnp::Schema::from<TestConfig>());

  // leaf carries a struct-literal default (value = 11, tag = "lit"), which wins over
  // TestLeafConfig's own field defaults (value = 3, tag = "leaf"). The 64-bit value is a string.
  EXPECT_EQ(defaults.at("leaf").at("value").get<std::string>(), "11");
  EXPECT_EQ(defaults.at("leaf").at("tag").get<std::string>(), "lit");
}

TEST(UtilsTest, decodeFromJsonDecodesFields)
{
  const auto schema = capnp::Schema::from<TestConfig>();
  const auto message = decodeFromJson(R"({"count": 5})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST(UtilsTest, decodeFromJsonIgnoresFieldsOutsideSchema)
{
  const auto schema = capnp::Schema::from<TestConfig>();

  // The stray field must not make the decode throw; the known field still decodes.
  const auto message = decodeFromJson(R"({"count": 5, "futureField": 9})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST(UtilsTest, writeJsonFileThenReadJsonFileRoundTrips)
{
  const auto path = testDirectory() / "roundTrip.json";
  const auto original = nlohmann::json{{"name", "value"}, {"nested", {{"count", 3}}}};

  writeJsonFile(path, original);

  EXPECT_EQ(readJsonFile(path), original);
}

TEST(UtilsTest, readJsonFileThrowsWhenFileMissing)
{
  EXPECT_THROW(
      static_cast<void>(readJsonFile(testDirectory() / "doesNotExist.json")),
      std::runtime_error);
}

TEST(UtilsTest, readJsonFileThrowsOnMalformedJson)
{
  const auto path = testDirectory() / "malformed.json";
  std::ofstream(path) << "{ not valid json";

  EXPECT_THROW(static_cast<void>(readJsonFile(path)), nlohmann::json::parse_error);
}

} // namespace
} // namespace nioc::terminus
