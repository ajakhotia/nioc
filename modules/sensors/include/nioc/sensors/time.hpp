////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/sensors/idl/timestamp.capnp.h>

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

/// @brief The instant @p timestamp records, as a nanosecond-resolution time point on NamedClock.
[[nodiscard]] NamedClock::time_point timePoint(Timestamp::Reader timestamp);

} // namespace nioc::sensors
