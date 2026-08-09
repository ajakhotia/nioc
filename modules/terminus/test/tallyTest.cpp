////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/common/time.hpp>
#include <nioc/terminus/tally.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace nioc::terminus
{
namespace
{

using common::NamedClock;

/// A 200 Hz stream, the rate a recorded inertial channel runs at.
constexpr auto kFastInterval = std::chrono::milliseconds{5};

/// A 1 Hz stream, slow enough it cannot be confused with the fast one.
constexpr auto kSlowInterval = std::chrono::seconds{1};

/// Samples per stream: enough that one interval more or less would show in the rate.
constexpr auto kSampleCount = 201;

/// A non-zero epoch offset, so a stream that silently started at zero would stand out.
constexpr auto kStart = NamedClock::time_point{std::chrono::seconds{100}};

/// Stand-in channels; the tally never interprets the value.
constexpr auto kImuChannel = chronicle::ChannelId{0x11ULL};
constexpr auto kImageChannel = chronicle::ChannelId{0x22ULL};

/// Observe @p count samples on @p channel, @p interval apart, starting at @p start.
void observeStream(
    Tally& tally,
    const chronicle::ChannelId channel,
    const NamedClock::time_point start,
    const NamedClock::duration interval,
    const int count)
{
  for(auto index = 0; index < count; ++index)
  {
    tally.observe(channel, start + (index * interval));
  }
}

/// Whether @p columns carries @p value.
bool has(const std::vector<std::string>& columns, const std::string_view value)
{
  return std::ranges::find(columns, value) != columns.end();
}

} // namespace

TEST(ChannelStatistics, ReportsTheNominalRateOfAnEvenlySpacedStream)
{
  auto tally = Tally{};

  observeStream(tally, kImuChannel, kStart, kFastInterval, kSampleCount);

  const auto& statistics = tally.at(kImuChannel);
  EXPECT_EQ(static_cast<std::uint64_t>(kSampleCount), statistics.count());
  EXPECT_DOUBLE_EQ(200.0, statistics.rate()); // a 5 ms interval is 200 Hz
  EXPECT_EQ(kFastInterval, statistics.shortestInterval());
  EXPECT_EQ(kFastInterval, statistics.longestInterval());
}

TEST(ChannelStatistics, ABackwardSampleIsCountedButLeftOutOfTheIntervals)
{
  auto tally = Tally{};

  tally.observe(kImuChannel, kStart);                       // t = 0 s
  tally.observe(kImuChannel, kStart + kSlowInterval);       // +1 s forward
  tally.observe(kImuChannel, kStart);                       // steps back, no interval
  tally.observe(kImuChannel, kStart + (2 * kSlowInterval)); // +2 s from the stepped-back sample

  // The backward sample is still the predecessor of the next one, so the second forward interval is
  // measured from it: 2 s, not 1 s. Both forward intervals count; the backward step does not.
  const auto& statistics = tally.at(kImuChannel);
  EXPECT_EQ(4U, statistics.count());
  EXPECT_EQ(kSlowInterval, statistics.shortestInterval());
  EXPECT_EQ(2 * kSlowInterval, statistics.longestInterval());
}

TEST(ChannelStatistics, ASingleSampleHasNoRateOrInterval)
{
  auto tally = Tally{};

  tally.observe(kImuChannel, kStart);

  const auto& statistics = tally.at(kImuChannel);
  EXPECT_EQ(1U, statistics.count());
  EXPECT_DOUBLE_EQ(0.0, statistics.rate());
  EXPECT_EQ(NamedClock::duration::zero(), statistics.shortestInterval());
  EXPECT_EQ(NamedClock::duration::zero(), statistics.longestInterval());
}

TEST(ChannelStatistics, headingsAndColumnsStayTheSameLength)
{
  const auto statistics = ChannelStatistics{};

  EXPECT_EQ(ChannelStatistics::headings().size(), statistics.columns().size());
}

TEST(ChannelStatistics, columnsCarryTheRenderedFigures)
{
  auto tally = Tally{};
  observeStream(tally, kImuChannel, kStart, kFastInterval, kSampleCount);

  const auto columns = tally.at(kImuChannel).columns();
  EXPECT_TRUE(has(columns, "201"));     // samples
  EXPECT_TRUE(has(columns, "200.000")); // rate
  EXPECT_TRUE(has(columns, "5.000"));   // shortest and longest interval, in ms
}

TEST(Tally, KeepsEachChannelSeparate)
{
  auto tally = Tally{};

  observeStream(tally, kImuChannel, kStart, kFastInterval, kSampleCount);
  observeStream(tally, kImageChannel, kStart, kSlowInterval, kSampleCount);

  EXPECT_DOUBLE_EQ(200.0, tally.at(kImuChannel).rate());
  EXPECT_DOUBLE_EQ(1.0, tally.at(kImageChannel).rate());
}

TEST(Tally, totalSamplesSumsAcrossChannels)
{
  auto tally = Tally{};

  constexpr auto kImuSamples = 10;
  constexpr auto kImageSamples = 6;
  observeStream(tally, kImuChannel, kStart, kFastInterval, kImuSamples);
  observeStream(tally, kImageChannel, kStart, kSlowInterval, kImageSamples);

  EXPECT_EQ(static_cast<std::uint64_t>(kImuSamples + kImageSamples), tally.totalSamples());
}

TEST(Tally, anUnobservedChannelIsAbsent)
{
  const auto tally = Tally{};

  EXPECT_FALSE(tally.contains(kImuChannel));
  EXPECT_EQ(0U, tally.totalSamples());
}

} // namespace nioc::terminus
