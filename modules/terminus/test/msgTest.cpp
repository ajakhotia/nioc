////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

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
