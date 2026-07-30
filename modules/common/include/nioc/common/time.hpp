////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <ratio>
#include <string>
#include <string_view>

namespace nioc::common
{

/// @brief The clock an instant read from a record is expressed against.
///
/// A tag type only: the time base is identified by whatever names it alongside the instant (UTC,
/// GPS, system, a sensor's own counter), and this asserts no relation to the host's clocks, so it
/// deliberately offers no now(). Its purpose is to keep a recorded instant from being mistaken for
/// one sampled here, which is exactly what a bare duration would fail to do.
///
/// Example:
///
///     const auto instant = NamedClock::time_point{std::chrono::nanoseconds{count}};
struct NamedClock
{
  using rep = std::int64_t;
  using period = std::nano;
  using duration = std::chrono::nanoseconds;
  using time_point = std::chrono::time_point<NamedClock>;

  /// Named time is read from records, not sampled from the host, so it never counts as steady.
  static constexpr bool is_steady = false;
};

/// @brief A `std::format` format string that renders a single time-point argument as an ISO 8601
/// UTC timestamp, e.g. `2026-06-24T13:45:07Z`.
///
/// Example:
///
///     std::format(kIso8601UtcFormat, std::chrono::system_clock::now());
///
/// The trailing `Z` is a literal in the format string; it is appended without converting the value,
/// so the time point you pass must already be expressed in UTC or the `Z` label will be wrong.
/// Prefer `iso8601UtcFormat` for `system_clock` time points.
///
/// @see iso8601UtcFormat
constexpr std::string_view kIso8601UtcFormat{"{:%FT%TZ}"};

/// @brief Format a system-clock time point as an ISO 8601 UTC timestamp string, e.g.
/// `2026-06-24T13:45:07Z`.
///
/// Example:
///
///     std::string stamp = iso8601UtcFormat(std::chrono::system_clock::now());
///
/// Sub-second digits appear only when the time point's representation carries them. `system_clock`
/// measures time from the UTC epoch, so the `Z` suffix is always correct here.
///
/// @return The timestamp as a newly allocated string.
///
/// @see kIso8601UtcFormat
inline std::string iso8601UtcFormat(const std::chrono::system_clock::time_point timePoint)
{
  return std::format(kIso8601UtcFormat, timePoint);
}

} // namespace nioc::common
