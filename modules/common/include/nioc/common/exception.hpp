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

/// @brief Throws @p Exception with a std::format message tagged by call site.
///
/// The message is `[<file>:<line>] <formatted message>`. The location is captured for you.
///
/// @tparam Exception Type to throw. Must be constructible from std::string (e.g.
/// std::runtime_error, std::invalid_argument, std::logic_error).
///
/// @param format Format string with `{}` placeholders. Pass a string literal; it captures the
/// call site. std::format has no formatter for std::filesystem::path, so pass `path.string()`,
/// not a path. The string is checked against @p args at compile time; a mismatch is a
/// compile error.
///
/// @param args Values for the placeholders, left to right.
///
/// @throws Exception Always.
///
/// @code
/// common::throwException<std::invalid_argument>("resource does not exist: {}", path.string());
/// // what(): "[port.cpp:259] resource does not exist: /tmp/missing"
/// @endcode
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
