////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/any.h>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace nioc::terminus
{
namespace
{
namespace fs = std::filesystem;

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class UtilsTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  UtilsTest(): ScratchDirectory{unitTestDirectory()} {}

protected:
  [[nodiscard]] fs::path testDirectory() const
  {
    return path();
  }
};

TEST_F(UtilsTest, encodeAsJsonRendersSchemaDefaults)
{
  const auto defaults = encodeAsJson(capnp::Schema::from<TestConfig>());

  EXPECT_EQ(defaults.at("name").get<std::string>(), ""); // Text with no default -> empty
  EXPECT_EQ(defaults.at("count").get<int>(), 7);
  EXPECT_TRUE(defaults.at("enabled").get<bool>());
  EXPECT_TRUE(defaults.at("gains").is_array());
  EXPECT_TRUE(defaults.at("gains").empty());    // List with no default -> empty array
  EXPECT_TRUE(defaults.at("leaf").is_object()); // nested struct -> nested object
}

TEST_F(UtilsTest, encodeAsJsonSurfacesStructLiteralDefaultsAndQuotes64BitIntegers)
{
  const auto defaults = encodeAsJson(capnp::Schema::from<TestConfig>());

  // leaf carries a struct-literal default (value = 11, tag = "lit"), which wins over
  // TestLeafConfig's own field defaults (value = 3, tag = "leaf"). The 64-bit value is a string.
  EXPECT_EQ(defaults.at("leaf").at("value").get<std::string>(), "11");
  EXPECT_EQ(defaults.at("leaf").at("tag").get<std::string>(), "lit");
}

TEST_F(UtilsTest, decodeFromJsonDecodesFields)
{
  const auto schema = capnp::Schema::from<TestConfig>();
  const auto message = decodeFromJson(R"({"count": 5})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST_F(UtilsTest, decodeFromJsonIgnoresFieldsOutsideSchema)
{
  const auto schema = capnp::Schema::from<TestConfig>();

  // The stray field must not make the decode throw; the known field still decodes.
  const auto message = decodeFromJson(R"({"count": 5, "futureField": 9})", schema);

  const auto config = message->getRoot<capnp::DynamicStruct>(schema).asReader().as<TestConfig>();
  EXPECT_EQ(config.getCount(), 5U);
}

TEST_F(UtilsTest, writeJsonFileThenReadJsonFileRoundTrips)
{
  const auto path = testDirectory() / "roundTrip.json";
  const auto original = nlohmann::json{{"name", "value"}, {"nested", {{"count", 3}}}};

  writeJsonFile(path, original);

  EXPECT_EQ(readJsonFile(path), original);
}

TEST_F(UtilsTest, readJsonFileThrowsWhenFileMissing)
{
  EXPECT_THROW(
      static_cast<void>(readJsonFile(testDirectory() / "doesNotExist.json")),
      std::runtime_error);
}

TEST_F(UtilsTest, readJsonFileThrowsOnMalformedJson)
{
  const auto path = testDirectory() / "malformed.json";
  std::ofstream(path) << "{ not valid json";

  EXPECT_THROW(static_cast<void>(readJsonFile(path)), nlohmann::json::parse_error);
}

TEST_F(UtilsTest, buildFieldNodeChainResolvesOneHandlePerSegment)
{
  const auto chain = buildFieldNodeChain(capnp::Schema::from<TestConfig>(), "leaf.value");
  if(not chain.has_value())
  {
    FAIL() << "The path did not resolve.";
  }

  ASSERT_EQ(chain->size(), 2U);
  EXPECT_EQ(std::string_view{chain->front().getProto().getName().cStr()}, "leaf");
  EXPECT_EQ(std::string_view{chain->back().getProto().getName().cStr()}, "value");
}

TEST_F(UtilsTest, dynamicFieldExtractorReadsNestedLeaf)
{
  constexpr auto kLeafValue = std::int64_t{42};

  const auto extractor = dynamicFieldExtractor<std::int64_t>(
      capnp::Schema::from<TestConfig>(),
      "leaf.value");
  if(not extractor.has_value())
  {
    FAIL() << "The path did not resolve.";
  }

  auto builder = capnp::MallocMessageBuilder{};
  builder.initRoot<TestConfig>().initLeaf().setValue(kLeafValue);

  EXPECT_EQ((*extractor)(builder.getRoot<capnp::AnyPointer>().asReader()), kLeafValue);
}

TEST_F(UtilsTest, dynamicFieldExtractorReadsTopLevelField)
{
  const auto extractor = dynamicFieldExtractor<std::uint32_t>(
      capnp::Schema::from<TestConfig>(),
      "count");
  if(not extractor.has_value())
  {
    FAIL() << "The path did not resolve.";
  }

  auto builder = capnp::MallocMessageBuilder{};
  builder.initRoot<TestConfig>().setCount(9U);

  EXPECT_EQ((*extractor)(builder.getRoot<capnp::AnyPointer>().asReader()), 9U);
}

TEST_F(UtilsTest, dynamicFieldExtractorRejectsMissingField)
{
  const auto schema = capnp::Schema::from<TestConfig>();

  EXPECT_FALSE(dynamicFieldExtractor<std::int64_t>(schema, "absent").has_value());
  EXPECT_FALSE(dynamicFieldExtractor<std::int64_t>(schema, "leaf.absent").has_value());
}

TEST_F(UtilsTest, dynamicFieldExtractorRejectsNonStructIntermediate)
{
  // `count` is a UInt32, so no path can descend through it.
  EXPECT_FALSE(
      dynamicFieldExtractor<std::int64_t>(capnp::Schema::from<TestConfig>(), "count.value")
          .has_value());
}

} // namespace
} // namespace nioc::terminus
