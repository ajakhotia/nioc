////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
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

TEST(UtilsTest, makeDefaultJsonMirrorsSchemaShapeWithDefaults)
{
  const auto defaults = makeDefaultJson(capnp::Schema::from<TestConfig>());

  EXPECT_EQ(defaults.at("name").get<std::string>(), ""); // Text with no default -> empty
  EXPECT_EQ(defaults.at("count").get<int>(), 7);
  EXPECT_TRUE(defaults.at("enabled").get<bool>());
  EXPECT_TRUE(defaults.at("gains").is_array());
  EXPECT_TRUE(defaults.at("gains").empty());    // List with no default -> empty array
  EXPECT_TRUE(defaults.at("leaf").is_object()); // nested struct -> nested object
}

TEST(UtilsTest, makeDefaultJsonSurfacesStructLiteralDefaultsAndQuotes64BitIntegers)
{
  const auto defaults = makeDefaultJson(capnp::Schema::from<TestConfig>());

  // leaf carries a struct-literal default (value = 11, tag = "lit"), which wins over
  // TestLeafConfig's own field defaults (value = 3, tag = "leaf"). The 64-bit value is a string.
  EXPECT_EQ(defaults.at("leaf").at("value").get<std::string>(), "11");
  EXPECT_EQ(defaults.at("leaf").at("tag").get<std::string>(), "lit");
}

TEST(UtilsTest, decodeMessageDecodesFieldsFromJson)
{
  const auto schema = capnp::Schema::from<TestConfig>();
  const auto message = decodeMessage(R"({"count": 5})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST(UtilsTest, decodeMessageIgnoresFieldsOutsideSchema)
{
  const auto schema = capnp::Schema::from<TestConfig>();

  // The stray field must not make the decode throw; the known field still decodes.
  const auto message = decodeMessage(R"({"count": 5, "futureField": 9})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST(UtilsTest, overrideFieldReplacesExistingField)
{
  auto tree = nlohmann::json::parse(R"({"name": "before", "count": 1})");

  overrideField(tree, nlohmann::json::json_pointer{"/name"}, "after");

  EXPECT_EQ(tree.at("name").get<std::string>(), "after");
  EXPECT_EQ(tree.at("count").get<int>(), 1); // sibling untouched
}

TEST(UtilsTest, overrideFieldReplacesNestedFieldKeepingSiblings)
{
  auto tree = nlohmann::json::parse(R"({"leaf": {"value": 1, "tag": "keep"}})");

  overrideField(tree, nlohmann::json::json_pointer{"/leaf/value"}, 9);

  EXPECT_EQ(tree.at("leaf").at("value").get<int>(), 9);
  EXPECT_EQ(tree.at("leaf").at("tag").get<std::string>(), "keep");
}

TEST(UtilsTest, overrideFieldWithNullDeletesField)
{
  auto tree = nlohmann::json::parse(R"({"name": "present", "count": 1})");

  overrideField(tree, nlohmann::json::json_pointer{"/name"}, nullptr);

  EXPECT_FALSE(tree.contains("name"));
  EXPECT_TRUE(tree.contains("count"));
}

TEST(UtilsTest, overrideFieldThrowsWhenFieldAbsent)
{
  auto tree = nlohmann::json::parse(R"({"name": "present"})");

  EXPECT_THROW(overrideField(tree, nlohmann::json::json_pointer{"/missing"}, 1), std::out_of_range);
}

TEST(UtilsTest, overrideFieldThrowsWhenNestedFieldAbsent)
{
  auto tree = nlohmann::json::parse(R"({"leaf": {"value": 1}})");

  EXPECT_THROW(
      overrideField(tree, nlohmann::json::json_pointer{"/leaf/missing"}, 1),
      std::out_of_range);
}

} // namespace
} // namespace nioc::terminus
