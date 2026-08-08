////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <nioc/terminus/schemaRegistry.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// A fresh directory for one test case, cleared of any previous run's leavings.
fs::path makeCaseDir(const std::string_view name)
{
  auto caseDir = fs::temp_directory_path() / "nioc-schemaRegistryTest" / name;
  fs::remove_all(caseDir);
  fs::create_directories(caseDir);
  return caseDir;
}

} // namespace

TEST(SchemaRegistry, recordedSchemasRoundTripAndReadAMessageDynamically)
{
  const auto caseDir = makeCaseDir("roundTrip");

  // Record the compiled schema and persist it, as a recording run does.
  {
    auto registry = SchemaRegistry{};
    registry.record<TestSchema>();
    registry.write(caseDir);
  }

  // Adopt it back, as a replay does; the compiled type plays no part from here on.
  const auto adopted = SchemaRegistry{caseDir};
  ASSERT_TRUE(adopted.contains(kSchemaId<TestSchema>));
  const auto schema = adopted.at(kSchemaId<TestSchema>);

  // Build a message with the compiled type, then read it back through the adopted schema alone.
  constexpr auto kValue = std::int64_t{86};
  constexpr auto kText = std::string_view{"dynamic"};

  auto builder = capnp::MallocMessageBuilder{};
  auto message = builder.initRoot<TestSchema>();
  message.setValue(kValue);
  message.setText(capnp::Text::Reader{kText.data(), kText.size()});

  const auto words = capnp::messageToFlatArray(builder);
  auto reader = capnp::FlatArrayMessageReader{words};
  const auto dynamic = reader.getRoot<capnp::AnyPointer>().getAs<capnp::DynamicStruct>(schema);
  EXPECT_EQ(dynamic.get("value").as<std::int64_t>(), kValue);
  EXPECT_EQ(std::string{dynamic.get("text").as<capnp::Text>().cStr()}, kText);
}

TEST(SchemaRegistry, anUnrecordedIdIsAbsentAndRefused)
{
  const auto registry = SchemaRegistry{};

  EXPECT_FALSE(registry.contains(kSchemaId<TestSchema>));
  EXPECT_THROW(static_cast<void>(registry.at(kSchemaId<TestSchema>)), std::out_of_range);
}

TEST(SchemaRegistry, anEmptyPathYieldsAnEmptyRegistry)
{
  const auto registry = SchemaRegistry{fs::path{}};

  EXPECT_FALSE(registry.contains(kSchemaId<TestSchema>));
}

TEST(SchemaRegistry, aRecordingWithoutSchemasYieldsAnEmptyRegistry)
{
  const auto registry = SchemaRegistry{makeCaseDir("noSchemas")};

  EXPECT_FALSE(registry.contains(kSchemaId<TestSchema>));
}

TEST(SchemaRegistry, aPortRecordsItsPublishersSchemasAndAReplayAdoptsThem)
{
  const auto workingDir = [&]
  {
    auto recordingDir = makeCaseDir("portRecords");
    auto port = Port{
        RunContext{std::move(recordingDir), {}, true, ""},
        [](Port& port, Port::Drivers&, Port::Components&, Port::Runners&)
        { static_cast<void>(port.publisher<TestSchema>("schemas")); }};
    return port.workingDir();
  }();

  const auto replay = Port{
      RunContext{workingDir / "replay", {}, false, "", workingDir},
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}};

  ASSERT_TRUE(replay.playbackSchemas().contains(kSchemaId<TestSchema>));
  EXPECT_EQ(
      replay.playbackSchemas().at(kSchemaId<TestSchema>).getProto().getId(),
      kSchemaId<TestSchema>);
}

} // namespace nioc::terminus
