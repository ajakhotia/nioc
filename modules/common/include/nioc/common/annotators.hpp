////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <source_location>
#include <string_view>

namespace nioc::common
{

/// @brief A format string paired with the source location of the call that built it.
///
/// Pass a string literal wherever one of these is expected: the literal converts implicitly, and
/// the conversion captures the call site. This lets a variadic function record where it was called
/// without a trailing source_location parameter or a macro.
///
/// The format string is held as a std::string_view, with no formatter details fixed in advance.
/// The consumer picks the formatter and matches the string to its arguments at run time.
struct FormatWithLocation
{
  /// @brief The format string, with no formatter details fixed in advance.
  std::string_view mFormat;

  /// @brief The source location of the call that built this wrapper.
  std::source_location mLocation;

  /// @brief Captures @p format and the call site.
  ///
  /// This constructor is consteval, so the @p format must be a compile-time constant — normally a
  /// string literal. The whole wrapper, captured location included, is built at compile time.
  ///
  /// @param format A compile-time-constant format string, normally a literal.
  ///
  /// @param location Defaulted to the call site; do not pass explicitly.
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  consteval FormatWithLocation(
      const char* format,
      const std::source_location& location = std::source_location::current()) noexcept:
    mFormat{format},
    mLocation{location}
  {
  }
};

} // namespace nioc::common
