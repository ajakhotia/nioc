////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <variant>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// A fresh empty directory under the system temp directory; prior contents are wiped.
fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-msgTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

} // namespace

TEST(MakeChannelId, isDeterministicAndSeparatesTopicsAndSchemas)
{
  const auto channel = makeChannelId(Msg<TestSchema>::kMsgId, "imu");

  // Equal inputs always map to the same channel.
  EXPECT_EQ(channel, makeChannelId(Msg<TestSchema>::kMsgId, "imu"));

  // A different topic or a different message type lands on a different channel.
  EXPECT_NE(channel, makeChannelId(Msg<TestSchema>::kMsgId, "gps"));
  EXPECT_NE(channel, makeChannelId(MsgId{Msg<TestSchema>::kMsgId.mValue + 1}, "imu"));
}

TEST(Msg, roundTripsThroughAChronicle)
{
  constexpr auto kValue = 86;
  constexpr auto kTopic = std::string_view{"roundTrip"};

  const auto logPath = [&]
  {
    auto writer = chronicle::Writer{makeFreshEmptyDir("roundTrip")};

    auto msg = Msg<TestSchema>{};
    msg.builder().setValue(kValue);
    write(msg, kTopic, writer);

    return writer.path();
  }();

  auto reader = chronicle::Reader{logPath};
  auto entry = reader.read();

  // The entry landed on the channel derived from the schema and topic, and carries the payload.
  EXPECT_EQ(entry.mChannelId, makeChannelId(Msg<TestSchema>::kMsgId, kTopic));

  const auto loaded = Msg<TestSchema>{std::move(entry.mMemoryCrate)};
  EXPECT_EQ(loaded.reader().getValue(), kValue);
}

TEST(Msg, stampsArrivalAndSequenceAtConstruction)
{
  const auto before = std::chrono::steady_clock::now();
  const auto msg = Msg<TestSchema>{};
  const auto after = std::chrono::steady_clock::now();

  EXPECT_GE(msg.arrivalTimestamp(), before);
  EXPECT_LE(msg.arrivalTimestamp(), after);
  EXPECT_EQ(msg.sequenceNumber(), 0U);
}

TEST(Msg, honorsAnExplicitArrivalAndSequence)
{
  const auto arrival = std::chrono::steady_clock::now() - std::chrono::seconds{5};
  constexpr auto kSequenceNumber = std::uint64_t{99};
  const auto msg = Msg<TestSchema>{arrival, kSequenceNumber};

  EXPECT_EQ(msg.arrivalTimestamp(), arrival);
  EXPECT_EQ(msg.sequenceNumber(), kSequenceNumber);
}

TEST(Msg, buildingThePayloadClearsTheGap)
{
  auto msg = Msg<TestSchema>{};

  // A freshly built message is a gap until its payload is allocated.
  EXPECT_TRUE(msg.isGap());
  msg.builder().setValue(1);
  EXPECT_FALSE(msg.isGap());
}

TEST(Msg, framingSurvivesARoundTrip)
{
  constexpr auto kValue = 7;
  constexpr auto kSequenceNumber = std::uint64_t{42};
  const auto arrival = std::chrono::steady_clock::now();

  const auto logPath = [&]
  {
    auto writer = chronicle::Writer{makeFreshEmptyDir("framing")};

    auto msg = Msg<TestSchema>{arrival, kSequenceNumber};
    msg.builder().setValue(kValue);
    write(msg, "framing", writer);

    return writer.path();
  }();

  auto reader = chronicle::Reader{logPath};
  const auto loaded = Msg<TestSchema>{reader.read().mMemoryCrate};

  EXPECT_FALSE(loaded.isGap());
  EXPECT_EQ(loaded.reader().getValue(), kValue);
  EXPECT_EQ(loaded.arrivalTimestamp(), arrival);
  EXPECT_EQ(loaded.sequenceNumber(), kSequenceNumber);
}

TEST(Msg, aNullMessageIsAGapThatRoundTrips)
{
  const auto arrival = std::chrono::steady_clock::now();
  constexpr auto kSequenceNumber = std::uint64_t{3};

  const auto logPath = [&]
  {
    auto writer = chronicle::Writer{makeFreshEmptyDir("gap")};

    // builder() is never called, so the payload stays null - a gap.
    const auto gap = Msg<TestSchema>{arrival, kSequenceNumber};
    EXPECT_TRUE(gap.isGap());
    write(gap, "gap", writer);

    return writer.path();
  }();

  auto reader = chronicle::Reader{logPath};
  const auto loaded = Msg<TestSchema>{reader.read().mMemoryCrate};

  // The gap and its framing survive the round trip.
  EXPECT_TRUE(loaded.isGap());
  EXPECT_EQ(loaded.sequenceNumber(), kSequenceNumber);
}

TEST(Msg, writingAMessageOpenedForReadingThrows)
{
  const auto logPath = [&]
  {
    auto writer = chronicle::Writer{makeFreshEmptyDir("writeReadOpened")};
    auto msg = Msg<TestSchema>{};
    msg.builder().setValue(1);
    write(msg, "topic", writer);
    return writer.path();
  }();

  auto reader = chronicle::Reader{logPath};
  const auto loaded = Msg<TestSchema>{reader.read().mMemoryCrate};

  // A message loaded from a crate is in its read state; it cannot be serialized again.
  auto secondWriter = chronicle::Writer{makeFreshEmptyDir("writeReadOpenedOut")};
  EXPECT_THROW(write(loaded, "topic", secondWriter), std::bad_variant_access);
}

} // namespace nioc::terminus
