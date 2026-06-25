////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <format>
#include <string>
#include <string_view>

namespace nioc::common
{

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
