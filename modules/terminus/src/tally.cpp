////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <nioc/common/time.hpp>
#include <nioc/terminus/tally.hpp>
#include <ranges>
#include <ratio>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace
{

// Intervals are held in the clock's own duration but reported in milliseconds, the scale at which
// sampling intervals and their spread are legible.
double toMilliseconds(const common::NamedClock::duration interval)
{
  return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(interval).count();
}

} // namespace

void ChannelStatistics::observe(const common::NamedClock::time_point sample)
{
  if(mCount == 0ULL)
  {
    mCount = 1ULL;
    mFirstSample = sample;
    mLastSample = sample;
    return;
  }

  const auto interval = sample - mLastSample;

  ++mCount;
  mLastSample = sample;

  // A sample that steps backwards has no meaningful interval against its predecessor; count it, but
  // leave the interval statistics alone.
  if(interval < common::NamedClock::duration::zero())
  {
    return;
  }

  mShortestInterval = std::min(mShortestInterval, interval);
  mLongestInterval = std::max(mLongestInterval, interval);
}

std::uint64_t ChannelStatistics::count() const noexcept
{
  return mCount;
}

double ChannelStatistics::rate() const noexcept
{
  // A single sample opens the span without closing it, so it carries no rate.
  const auto span = std::chrono::duration<double>{mLastSample - mFirstSample};
  return (mCount > 1ULL and span.count() > 0.0) ? static_cast<double>(mCount - 1ULL) / span.count()
                                                : 0.0;
}

common::NamedClock::duration ChannelStatistics::shortestInterval() const noexcept
{
  return (mCount > 1ULL) ? mShortestInterval : common::NamedClock::duration::zero();
}

common::NamedClock::duration ChannelStatistics::longestInterval() const noexcept
{
  return mLongestInterval;
}

std::vector<std::string> ChannelStatistics::headings()
{
  return {"samples", "rate[Hz]", "minInterval[ms]", "maxInterval[ms]"};
}

std::vector<std::string> ChannelStatistics::columns() const
{
  return {
      std::format("{}", mCount),
      std::format("{:.3f}", rate()),
      std::format("{:.3f}", toMilliseconds(shortestInterval())),
      std::format("{:.3f}", toMilliseconds(longestInterval()))};
}

void Tally::observe(const chronicle::ChannelId channel, const common::NamedClock::time_point sample)
{
  (*this)[channel].observe(sample);
}

std::uint64_t Tally::totalSamples() const
{
  auto total = std::uint64_t{0ULL};
  for(const auto& statistics: *this | std::views::values)
  {
    total += statistics.count();
  }
  return total;
}

} // namespace nioc::terminus
