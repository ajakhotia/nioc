////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <nioc/chronicle/defines.hpp>
#include <nioc/common/time.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace nioc::terminus
{

/// @brief The timing statistics of one channel: how many samples it carried and, from the instants
/// of those samples, its rate and the spread of the intervals between them.
///
/// Feed it the timestamp of each message in arrival order with @ref observe; read the accumulated
/// figures with the accessors, or as a row of strings with @ref columns under @ref headings. The
/// instants are the message timestamps, not wall-clock, so the figures describe the recorded stream
/// itself, independent of how fast it is replayed. An instant that steps backwards against its
/// predecessor is still counted, but its interval is left out, so one out-of-order sample cannot
/// poison the rate.
///
/// Everything about how these statistics render lives here, beside the accumulation: add a figure
/// and its heading, its formatting, and its update all sit together.
class ChannelStatistics
{
public:
  /// @brief Fold one sample instant into the statistics.
  ///
  /// @param sample The instant the message carries.
  void observe(common::NamedClock::time_point sample);

  /// @brief Number of samples observed.
  [[nodiscard]] std::uint64_t count() const noexcept;

  /// @brief Mean rate over the observed span, in hertz; zero with fewer than two samples.
  [[nodiscard]] double rate() const noexcept;

  /// @brief Shortest interval seen between consecutive samples; zero with fewer than two.
  [[nodiscard]] common::NamedClock::duration shortestInterval() const noexcept;

  /// @brief Longest interval seen between consecutive samples; zero with fewer than two.
  [[nodiscard]] common::NamedClock::duration longestInterval() const noexcept;

  /// @brief The column headings these statistics render under, in order.
  ///
  /// Paired with @ref columns: the two must stay the same length and order, which a test pins.
  [[nodiscard]] static std::vector<std::string> headings();

  /// @brief These statistics as one row of strings, in the same order as @ref headings.
  [[nodiscard]] std::vector<std::string> columns() const;

private:
  /// Number of samples observed.
  std::uint64_t mCount{0ULL};

  /// The first sample, anchoring the span the channel covers.
  common::NamedClock::time_point mFirstSample;

  /// The most recent sample: both the far end of the span and the predecessor the next interval is
  /// measured against.
  common::NamedClock::time_point mLastSample;

  /// Shortest forward interval seen.
  common::NamedClock::duration mShortestInterval{common::NamedClock::duration::max()};

  /// Longest forward interval seen.
  common::NamedClock::duration mLongestInterval{common::NamedClock::duration::zero()};
};

/// @brief Per-channel timing statistics: the @ref ChannelStatistics of every channel observed,
/// keyed by channel.
///
/// It is a `std::unordered_map<ChannelId, ChannelStatistics>`: iteration, `size`, `at`, `contains`,
/// and the rest come straight from the map. Feed samples with @ref observe, which finds or creates
/// the channel's statistics; a channel first seen here springs into existence with an empty tally.
/// It holds only the timing; a channel's identity (its topic and schema) is read from the
/// @ref TopicRegistry when a report needs it, not duplicated here.
///
/// Example:
///
///     auto tally = Tally{};
///     tally.observe(channelId, message.header().timestamp());
///     std::cout << tally.at(channelId).rate();
class Tally: public std::unordered_map<chronicle::ChannelId, ChannelStatistics>
{
public:
  /// @brief Fold one sample into @p channel's statistics, creating them on first sight.
  ///
  /// @param channel The channel the sample was delivered on.
  ///
  /// @param sample The instant the message carries.
  void observe(chronicle::ChannelId channel, common::NamedClock::time_point sample);

  /// @brief Total samples observed across every channel.
  [[nodiscard]] std::uint64_t totalSamples() const;
};

} // namespace nioc::terminus
