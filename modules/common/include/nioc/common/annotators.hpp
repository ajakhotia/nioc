////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <source_location>

namespace nioc::common
{

/// @brief A format string plus the source location of the call that built it.
///
/// Pass a string literal where one of these is expected; it converts implicitly. The literal is
/// checked against @p FormatString's argument types at compile time, and the call site is captured.
/// A variadic function can thus record where it was called without a trailing source_location
/// parameter or a macro. A format string that does not match its arguments fails to compile.
///
/// @tparam FormatString The format-string type. It fixes the expected argument types and checks the
/// literal against them. Pass `std::format_string<Args...>` or a compatible type such as
/// `fmt::format_string<Args...>`.
template<typename FormatString>
struct FormatWithLocation
{
  /// @brief The format string, already checked against @p FormatString's argument types.
  FormatString mFormat;

  /// @brief The source location of the call that built this wrapper.
  std::source_location mLocation;

  /// @brief Stores @p format and the call site, checking @p format at compile time.
  ///
  /// consteval, so the @p format string must be a compile-time constant (normally a string
  /// literal). It is checked against @p FormatString's argument types. The whole wrapper, location
  /// included, is built at compile time.
  ///
  /// @param format A compile-time-constant format string, normally a literal.
  ///
  /// @param location Defaulted to the call site. Do not pass it.
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
