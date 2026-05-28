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

/// @brief Throws @p Exception, its message built with std::format and tagged with the call site.
///
/// The thrown message is `[<file>:<line>] <formatted message>`, with the location captured
/// automatically — so a hand-written "[Module] " prefix on the message is unnecessary.
///
/// @tparam Exception Exception to throw; must be constructible from a std::string
/// (std::runtime_error, std::invalid_argument, std::logic_error, ...).
///
/// @param format Format string with `{}` placeholders. A string literal converts to it and
/// captures the call site. std::format has no formatter for std::filesystem::path, so pass
/// `path.string()` rather than a path. The string is matched to @p args at run time; a mismatch
/// raises std::format_error.
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @throws Exception Always.
///
/// @code
/// common::throwException<std::invalid_argument>("resource does not exist: {}", path.string());
/// // what(): "[port.cpp:259] resource does not exist: /tmp/missing"
/// @endcode
template<typename Exception, typename... Args>
[[noreturn]] void throwException(const FormatWithLocation& format, const Args&... args)
{
  throw Exception{ std::format(
      "[{}:{}] {}",
      std::filesystem::path{ format.mLocation.file_name() }.filename().string(),
      format.mLocation.line(),
      std::vformat(format.mFormat, std::make_format_args(args...))) };
}

} // namespace nioc::common
