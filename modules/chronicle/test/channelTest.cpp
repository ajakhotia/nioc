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

std::vector<std::byte> makeBytes(const std::size_t size, const unsigned char start = 0U)
{
  auto bytes = std::vector<std::byte>(size);
  for(auto index = std::size_t{0}; index < size; ++index)
  {
    bytes[index] = std::byte(static_cast<unsigned char>(start + index));
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

// Reserves, fills, and records one frame, returning a crate that keeps the roll bytes alive.
Crate record(Channel& channel, const std::span<const std::byte> data)
{
  auto reservation = channel.reserve(data.size());
  std::memcpy(reservation.span().data(), data.data(), data.size());
  return Crate{std::move(reservation), data.size()};
}

} // namespace

TEST(Channel, recordedFrameLandsInRollAndTimeline)
{
  const auto dir = freshDir("chRecord");
  const auto data = makeBytes(20, 1);

  auto crate = Crate{};
  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kRollCapacity, timeline};
    crate = record(channel, data);
    timeline.shrink_to_fit(); // trim to the written entries for the read-back below
  } // channel + timeline destroyed: the active roll is trimmed too

  // The crate still views the frame's bytes (it keeps the roll mapping alive).
  EXPECT_TRUE(std::ranges::equal(crate.span(), std::as_bytes(std::span{data})));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].mChannelId, channelA);
  EXPECT_EQ(entries[0].mRollId, 0U);
  EXPECT_EQ(entries[0].mOffset, 0U);
  EXPECT_EQ(entries[0].mSize, data.size());

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

TEST(Channel, rollsOverToANewRollWhenFull)
{
  const auto dir = freshDir("chRollover");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto frame = makeBytes(100, 7); // 104 word-aligned; two will not share a 128-byte roll

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};
    static_cast<void>(record(channel, frame)); // roll 0
    static_cast<void>(record(channel, frame)); // roll 1
    timeline.shrink_to_fit();
  }

  EXPECT_TRUE(fs::exists(dir / "chanA" / buildRollName(0)));
  EXPECT_TRUE(fs::exists(dir / "chanA" / buildRollName(1)));

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries[0].mRollId, 0U);
  EXPECT_EQ(entries[1].mRollId, 1U);
}

TEST(Channel, aFrameLargerThanTheRollCapacityGetsItsOwnRoll)
{
  const auto dir = freshDir("chBig");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto big = makeBytes(300, 3);

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};
    static_cast<void>(record(channel, big));
  }

  const auto roll = containers::MmapConstArray<std::byte>{dir / "chanA" / buildRollName(0)};
  EXPECT_GE(roll.size(), big.size()); // the oversized frame fit in its own roll
  EXPECT_TRUE(std::ranges::equal(std::span{roll}.first(big.size()), std::as_bytes(std::span{big})));
}

TEST(Channel, reclaimsTheUnusedTailOfAnOverReservation)
{
  const auto dir = freshDir("chReclaim");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto frame = makeBytes(8, 1);

  {
    auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
    auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};

    // Reserve generously but use only a few bytes. Two 104-byte claims would not share a 128-byte
    // roll, so if the unused tail were not reclaimed the second frame would land in roll 1.
    for(auto round = 0; round < 2; ++round)
    {
      auto reservation = channel.reserve(100);
      std::memcpy(reservation.span().data(), frame.data(), frame.size());
      static_cast<void>(Crate{std::move(reservation), frame.size()});
    }
    timeline.shrink_to_fit();
  }

  const auto entries = readEntries(dir / kTimelineFileName);
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries[0].mRollId, 0U);
  EXPECT_EQ(entries[0].mOffset, 0U);
  EXPECT_EQ(entries[1].mRollId, 0U); // the second frame reused the same roll
  EXPECT_EQ(entries[1].mOffset, 8U); // abutting the first frame's word-aligned end
  EXPECT_FALSE(fs::exists(dir / "chanA" / buildRollName(1)));
}

TEST(Channel, aCrateStaysReadableAfterTheChannelRollsToANewRoll)
{
  const auto dir = freshDir("chWeak");
  constexpr auto kTinyRoll = std::size_t{128};
  const auto first = makeBytes(100, 1);
  const auto second = makeBytes(100, 200);

  auto timeline = TimelineTape{dir / kTimelineFileName, kTimelineEntries};
  auto channel = Channel{channelA, dir / "chanA", kTinyRoll, timeline};

  const auto crateA = record(channel, first);  // lives in roll 0
  const auto crateB = record(channel, second); // forces roll 1; roll 0 now owned only by crateA

  // crateA still views roll 0's bytes even though the channel has moved on to roll 1.
  EXPECT_TRUE(std::ranges::equal(crateA.span(), std::as_bytes(std::span{first})));
  EXPECT_TRUE(std::ranges::equal(crateB.span(), std::as_bytes(std::span{second})));
}

} // namespace nioc::chronicle
