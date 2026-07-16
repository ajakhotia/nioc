////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/sensors/idl/timestamp.capnp.h>
#include <string_view>

namespace nioc::sensors
{

/// @brief The clock sensor timestamps are expressed against.
///
/// A tag type only: the clock is identified solely by the Timestamp's reference field (UTC, GPS,
/// system, ...) and asserts no relation to the host's clocks, so it deliberately offers no now().
struct NamedClock
{
  using rep = std::int64_t;
  using period = std::nano;
  using duration = std::chrono::nanoseconds;
  using time_point = std::chrono::time_point<NamedClock>;

  /// Named time is read from records, not sampled from the host, so it never counts as steady.
  static constexpr bool is_steady = false;
};

/// @brief The instant @p reader records, as a nanosecond-resolution time point on NamedClock.
[[nodiscard]] NamedClock::time_point timePoint(Timestamp::Reader reader);

/// @brief Record @p timePoint into @p builder as its nanosecond count since the epoch, naming the
/// clock it is read against.
///
/// @param builder The Timestamp record to fill.
///
/// @param timePoint The instant to record.
///
/// @param reference The clock the instant is expressed in (for example "UTC", "GPS", "system");
/// empty marks it unspecified.
void setTimePoint(
    Timestamp::Builder builder,
    NamedClock::time_point timePoint,
    std::string_view reference);

} // namespace nioc::sensors
