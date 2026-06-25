////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <source_location>

namespace nioc::common
{

/// @brief A struct that is implicitly constructible from a format string, and that quietly
/// captures the source location (file + line number) of that format string.
///
/// Example:
///
///     // Takes a format string plus, automatically, the caller's file and line.
///     void logLine(FormatWithLocation<std::format_string<int, int>> fmt, int a, int b);
///
///     logLine("The value of A is {} and B is {}.", 7, 8);
///     fmt.mLocation points at THIS line, not at logLine.
///
/// @tparam FormatString The format-string type; can be validated using std::format at compile
/// time.
///
/// @see throwException, logAt
template<typename FormatString>
struct FormatWithLocation
{
  /// The format string itself.
  FormatString mFormat;

  /// The file and line where the format string was written. Defaults to the caller's location.
  std::source_location mLocation;

  /// @brief Implicitly converts a `const char*` string literal into a `FormatWithLocation`, while
  /// the default @p location captures the call site.
  ///
  /// @param format The format string. Must be known at compile-time.
  ///
  /// @param location Defaults to the call site. Leave it defaulted; do not pass it explicitly.
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  consteval FormatWithLocation(
      const char* format,
      const std::source_location& location = std::source_location::current()):
    mFormat{format},
    mLocation{location}
  {
  }
};

} // namespace nioc::common
