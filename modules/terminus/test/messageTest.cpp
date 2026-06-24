////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/reader.hpp>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/draft.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

chronicle::ChannelId channelFor(const std::string_view topic)
{
  return chronicle::makeChannelId(kSchemaId<TestSchema>, topic);
}

/// A recording Port under a fresh per-test temp root; its publishers mint the drafts under test.
Port makePort(const std::string_view name)
{
  return Port{
      Manifest{
               RunContext{fs::temp_directory_path() / "nioc-messageTest" / name, {}, true, ""},
               ConfigStore{{}, {}}},
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}
  };
}

} // namespace

TEST(Message, draftBuildsReadsAndRoundTripsThroughAChronicle)
{
  constexpr auto kValue = std::int64_t{86};
  constexpr auto kTopic = std::string_view{"roundTrip"};

  // Record one message, capturing the publisher-assigned arrival and sequence to compare against
  // the reloaded copy.
  const auto [chronicleDir, arrival, sequenceNumber] = [&]
  {
    auto port = makePort("roundTrip");
    auto publisher = port.publisher<TestSchema>(kTopic);

    auto draft = publisher.draft();
    draft.builder().setValue(kValue);

    const Message<TestSchema> message = std::move(draft);
    EXPECT_FALSE(message.isGap());
    EXPECT_EQ(message.reader().getValue(), kValue);

    return std::tuple{
        port.workingDir() / "chronicle",
        message.arrivalTimestamp(),
        message.sequenceNumber()};
  }();

  auto reader = chronicle::Reader{chronicleDir};
  auto entry = *reader.begin();

  EXPECT_EQ(entry.mChannelId, channelFor(kTopic));

  const auto loaded = Message<TestSchema>{std::move(entry.mCrate)};
  EXPECT_FALSE(loaded.isGap());
  EXPECT_EQ(loaded.reader().getValue(), kValue);
  EXPECT_EQ(loaded.arrivalTimestamp(), arrival);
  EXPECT_EQ(loaded.sequenceNumber(), sequenceNumber);
}

TEST(Message, anUnbuiltDraftIsAGapThatRoundTrips)
{
  const auto [chronicleDir, sequenceNumber] = [&]
  {
    auto port = makePort("gap");
    auto publisher = port.publisher<TestSchema>("gap");

    // builder() is never called, so the payload stays null - a gap.
    auto draft = publisher.draft();
    const Message<TestSchema> message = std::move(draft);
    EXPECT_TRUE(message.isGap());

    return std::tuple{port.workingDir() / "chronicle", message.sequenceNumber()};
  }();

  auto reader = chronicle::Reader{chronicleDir};
  const auto loaded = Message<TestSchema>{(*reader.begin()).mCrate};

  EXPECT_TRUE(loaded.isGap());
  EXPECT_EQ(loaded.sequenceNumber(), sequenceNumber);
}

TEST(Message, aBustedReservationFlattensAndRoundTrips)
{
  constexpr auto kValue = std::int64_t{1234};

  const auto chronicleDir = [&]
  {
    auto port = makePort("busted");
    auto publisher = port.publisher<TestSchema>("busted");

    // An 8-byte reservation cannot hold even the envelope, forcing the build onto the heap; the
    // draft-to-message conversion must re-root it into a fresh single-segment frame.
    auto draft = publisher.draft(8);
    draft.builder().setValue(kValue);
    const Message<TestSchema> message = std::move(draft);
    EXPECT_EQ(message.reader().getValue(), kValue);

    return port.workingDir() / "chronicle";
  }();

  auto reader = chronicle::Reader{chronicleDir};
  const auto loaded = Message<TestSchema>{(*reader.begin()).mCrate};
  EXPECT_FALSE(loaded.isGap());
  EXPECT_EQ(loaded.reader().getValue(), kValue);
}

TEST(Message, moveConstructsAndReadsAfterTheSourceIsDestroyed)
{
  constexpr auto kValue = std::int64_t{55};

  auto port = makePort("move");
  auto publisher = port.publisher<TestSchema>("move");

  // Move-construct out of an inner scope so the moved-from message - and the reader it owned - is
  // destroyed before the read below; a member-wise (dangling) move would fault here.
  const auto moved = [&]
  {
    auto draft = publisher.draft();
    draft.builder().setValue(kValue);
    Message<TestSchema> source = std::move(draft);
    return Message<TestSchema>{std::move(source)};
  }();

  EXPECT_FALSE(moved.isGap());
  EXPECT_EQ(moved.reader().getValue(), kValue);
}

TEST(Message, aMultiSegmentBuildFlattensAndRoundTrips)
{
  constexpr auto kValue = std::int64_t{99};
  constexpr auto kCount = std::size_t{2000};
  const auto text = std::string(2000, 'x');

  const auto chronicleDir = [&]
  {
    auto port = makePort("multiSegment");
    auto publisher = port.publisher<TestSchema>("multiSegment");

    // The default reservation fits the envelope and root, but the large text and list spill onto
    // heap segments - a genuine multi-segment build, whose cross-segment far pointers the re-root
    // must collapse (unlike the all-heap case a tiny reservation produces).
    auto draft = publisher.draft();
    auto builder = draft.builder();
    builder.setValue(kValue);
    builder.setText(text.c_str());
    auto numbers = builder.initNumbers(static_cast<unsigned int>(kCount));
    for(auto i = std::size_t{0}; i < kCount; ++i)
    {
      numbers.set(static_cast<unsigned int>(i), static_cast<std::int64_t>(i));
    }

    const Message<TestSchema> message = std::move(draft);
    EXPECT_EQ(message.reader().getValue(), kValue);

    return port.workingDir() / "chronicle";
  }();

  auto reader = chronicle::Reader{chronicleDir};
  const auto loaded = Message<TestSchema>{(*reader.begin()).mCrate};
  const auto payload = loaded.reader();

  EXPECT_FALSE(loaded.isGap());
  EXPECT_EQ(payload.getValue(), kValue);
  EXPECT_EQ(std::string{payload.getText().cStr()}, text);
  ASSERT_EQ(payload.getNumbers().size(), kCount);
  for(auto i = std::size_t{0}; i < kCount; ++i)
  {
    ASSERT_EQ(payload.getNumbers()[static_cast<unsigned int>(i)], static_cast<std::int64_t>(i));
  }
}

} // namespace nioc::terminus
