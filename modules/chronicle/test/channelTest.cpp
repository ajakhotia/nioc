////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/channel.hpp>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/defines.hpp>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <nioc/containers/tape.hpp>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace nioc::chronicle
{
namespace
{
namespace fs = std::filesystem;

using TimelineTape = containers::Tape<containers::MmapArray<TimelineEntry>>;

constexpr auto channelA = ChannelId{16983ULL};
constexpr auto kRollCapacity = std::size_t{4096};
constexpr auto kTimelineEntries = std::size_t{64}; // far more than any test below records

fs::path freshDir(const std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

std::vector<std::byte> makeBytes(const std::size_t size, const std::byte start = std::byte{0})
{
  auto bytes = std::vector<std::byte>(size);
  for(auto index = std::size_t{0}; index < size; ++index)
  {
    bytes.at(index) = std::byte(
        static_cast<unsigned char>(std::to_integer<unsigned char>(start) + index));
  }
  return bytes;
}

// Reads every entry back from the timeline @p file, in slot order.
std::vector<TimelineEntry> readEntries(const fs::path& file)
{
  if(fs::file_size(file) == 0)
  {
    return {}; // a trimmed-empty file holds no entries and cannot be mapped
  }
  const auto array = containers::MmapConstArray<TimelineEntry>{file};
  return {array.begin(), array.end()};
}

} // namespace

TEST(Channel, recordedFrameLandsInRollAndTimeline)
{
  const auto dir = freshDir("chRecord");
  const auto data = makeBytes(20, std::byte{1});

  auto crate = Crate{};
  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kRollCapacity, timeline};
    crate = channel.write(data);
    timeline.shrink_to_fit(); // trim to the written entries for the read-back below
  } // channel + timeline destroyed: the active roll is trimmed too

  // The crate still views the frame's bytes (it keeps the roll mapping alive).
  EXPECT_TRUE(std::ranges::equal(crate.span(), std::as_bytes(std::span{data})));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries.at(0).mChannelId, channelA);
  EXPECT_EQ(entries.at(0).mRollId, 0U);
  EXPECT_EQ(entries.at(0).mOffset, 0U);
  EXPECT_EQ(entries.at(0).mSize, data.size());

  const auto roll = containers::MmapConstArray<std::byte>{dir / "chanA" / buildRollName(0)};
  ASSERT_GE(roll.size(), data.size());
  EXPECT_TRUE(
      std::ranges::equal(std::span{roll}.first(data.size()), std::as_bytes(std::span{data})));
}

TEST(Channel, anAbandonedReservationRecordsNothing)
{
  const auto dir = freshDir("chAbandon");

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kRollCapacity, timeline};

    auto reservation = channel.reserve(16); // built but never made into a crate
    static_cast<void>(reservation.span());
    timeline.shrink_to_fit();
  }

  EXPECT_TRUE(readEntries(dir / kTimelineFileName).empty());
}

TEST(Channel, anAbandonedReservationRewindsSoItsSpaceIsReused)
{
  const auto dir = freshDir("chAbandonRewind");
  auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
  auto channel = Channel{channelA, dir / "chanA", kRollCapacity, timeline};

  const std::byte* start = nullptr;
  {
    auto reservation = channel.reserve(64); // claims [0, 64)
    start = reservation.span().data();
  } // abandoned (never made into a crate): the destructor rewinds the claim off the active roll

  const auto reused = channel.reserve(32);
  EXPECT_EQ(reused.span().data(), start); // reused the rewound space rather than starting after it
}

TEST(Channel, rollsOverToANewRollWhenFull)
{
  const auto dir = freshDir("chRollover");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto frame = makeBytes(
      100,
      std::byte{7}); // 104 word-aligned; two will not share a 128-byte roll

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};
    static_cast<void>(channel.write(frame)); // roll 0
    static_cast<void>(channel.write(frame)); // roll 1
    timeline.shrink_to_fit();
  }

  EXPECT_TRUE(fs::exists(dir / "chanA" / buildRollName(0)));
  EXPECT_TRUE(fs::exists(dir / "chanA" / buildRollName(1)));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries.at(0).mRollId, 0U);
  EXPECT_EQ(entries.at(1).mRollId, 1U);
}

TEST(Channel, aFrameLargerThanTheRollCapacityGetsItsOwnRoll)
{
  const auto dir = freshDir("chBig");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto big = makeBytes(300, std::byte{3});

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};
    static_cast<void>(channel.write(big));
  }

  const auto roll = containers::MmapConstArray<std::byte>{dir / "chanA" / buildRollName(0)};
  EXPECT_GE(roll.size(), big.size()); // the oversized frame fit in its own roll
  EXPECT_TRUE(std::ranges::equal(std::span{roll}.first(big.size()), std::as_bytes(std::span{big})));
}

TEST(Channel, reclaimsTheUnusedTailOfAnOverReservation)
{
  const auto dir = freshDir("chReclaim");
  constexpr auto kTinyRoll = std::size_t{64};
  const auto frame = makeBytes(8, std::byte{1});

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};

    // Reserve more than each frame uses, then commit only a few bytes so the over-reservation's
    // unused tail is reclaimed and the next frame reuses the same roll instead of a fresh one. The
    // reserve size is load-bearing relative to kTinyRoll: it exceeds half the roll, so without
    // reclaim two claims would overflow into a second roll -- precisely what this test rules out --
    // yet it still fits once the tail is reclaimed.
    for(auto round = 0; round < 2; ++round)
    {
      auto reservation = channel.reserve(48);
      std::memcpy(reservation.span().data(), frame.data(), frame.size());
      static_cast<void>(std::move(reservation).commit(frame.size()));
    }
    timeline.shrink_to_fit();
  }

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries.at(0).mRollId, 0U);
  EXPECT_EQ(entries.at(0).mOffset, 0U);
  EXPECT_EQ(entries.at(1).mRollId, 0U); // the second frame reused the same roll
  EXPECT_EQ(entries.at(1).mOffset, 8U); // abutting the first frame's word-aligned end
  EXPECT_FALSE(fs::exists(dir / "chanA" / buildRollName(1)));
}

TEST(Channel, modifyGrowsAReservationInTheSameRollWhenItFits)
{
  const auto dir = freshDir("chModifyFit");
  const auto frame = makeBytes(80, std::byte{5});

  auto crate = Crate{};
  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kRollCapacity, timeline};

    auto reservation = channel.reserve(16); // start small
    reservation.modify(frame.size());       // grow; fits in the 4096-byte roll, same start
    ASSERT_GE(reservation.span().size(), frame.size());
    std::memcpy(reservation.span().data(), frame.data(), frame.size());
    crate = std::move(reservation).commit(frame.size());
    timeline.shrink_to_fit();
  }

  EXPECT_TRUE(std::ranges::equal(crate.span(), std::as_bytes(std::span{frame})));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries.at(0).mRollId, 0U); // still the first roll
  EXPECT_EQ(entries.at(0).mOffset, 0U); // re-claimed at the released start
  EXPECT_EQ(entries.at(0).mSize, frame.size());
}

TEST(Channel, modifyRollsOverWhenTheNewSizeNoLongerFits)
{
  const auto dir = freshDir("chModifyRoll");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto big = makeBytes(300, std::byte{9}); // larger than the 128-byte roll

  auto crate = Crate{};
  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};

    auto reservation = channel.reserve(16); // fits in roll 0
    reservation.modify(big.size());         // 300 > 128: rolls over to a fresh roll
    ASSERT_GE(reservation.span().size(), big.size());
    std::memcpy(reservation.span().data(), big.data(), big.size());
    crate = std::move(reservation).commit(big.size());
    timeline.shrink_to_fit();
  }

  EXPECT_TRUE(std::ranges::equal(crate.span(), std::as_bytes(std::span{big})));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries.at(0).mRollId, 1U); // the grow moved off roll 0 onto its own roll
  EXPECT_EQ(entries.at(0).mSize, big.size());
  EXPECT_TRUE(fs::exists(dir / "chanA" / buildRollName(1)));
}

TEST(Channel, aCrateStaysReadableAfterTheChannelRollsToANewRoll)
{
  const auto dir = freshDir("chWeak");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto first = makeBytes(100, std::byte{1});
  const auto second = makeBytes(100, std::byte{200});

  auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
  auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};

  const auto crateA = channel.write(first);  // lives in roll 0
  const auto crateB = channel.write(second); // forces roll 1; roll 0 now owned only by crateA

  // crateA still views roll 0's bytes even though the channel has moved on to roll 1.
  EXPECT_TRUE(std::ranges::equal(crateA.span(), std::as_bytes(std::span{first})));
  EXPECT_TRUE(std::ranges::equal(crateB.span(), std::as_bytes(std::span{second})));
}

} // namespace nioc::chronicle
