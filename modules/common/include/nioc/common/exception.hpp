////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <format>
#include <nioc/common/annotators.hpp>

namespace nioc::common
{

/// @brief Throws an `Exception` whose message is the formatted text, prefixed with the file name
/// and line of the call site.
///
/// The exception's message is `"[<file>:<line>] <formatted-message>"`, where `<file>` is the base
/// file name (no directory) and `<line>` is the line of the call. The call site is captured
/// automatically by @p format; you do not pass it.
///
/// Example:
///
///     // Throws std::runtime_error{"[widget.cpp:42] bad id 7, expected < 5"}.
///     throwException<std::runtime_error>("bad id {}, expected < {}", 7, 5);
///
/// @tparam Exception The exception type to throw. Must be specified explicitly (it cannot be
/// deduced) and must be constructible from a `std::string`.
///
/// @tparam Args The types of the format arguments, deduced from @p args.
///
/// @param format The format string, with the call site captured implicitly. Validated against
/// @p args at compile time, so type mismatches are compile errors, not runtime throws.
///
/// @param args The values substituted into @p format.
///
/// @throws Exception Always; this function never returns normally.
///
/// @see FormatWithLocation
template<typename Exception, typename... Args>
[[noreturn]] void throwException(
    const FormatWithLocation<std::format_string<Args...>>& format,
    const Args&... args)
{
  // `format` already validated the string against Args at the call site, so use the lighter
  // type-erased vformat here.
  throw Exception{std::format(
      "[{}:{}] {}",
      std::filesystem::path{format.mLocation.file_name()}.filename().string(),
      format.mLocation.line(),
      std::vformat(format.mFormat.get(), std::make_format_args(args...)))};
}

} // namespace nioc::common
