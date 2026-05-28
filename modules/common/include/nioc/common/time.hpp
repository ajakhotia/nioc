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

/// @brief std::format specification for an ISO 8601 UTC date-time.
constexpr std::string_view kIso8601UtcFormat{ "{:%FT%TZ}" };

/// @brief Formats a system_clock time point as an ISO 8601 UTC date-time string.
///
/// Example output: `2025-09-01T14:18:33.992295120Z`.
inline std::string iso8601UtcFormat(const std::chrono::system_clock::time_point timePoint)
{
  return std::format(kIso8601UtcFormat, timePoint);
}

} // namespace nioc::common
