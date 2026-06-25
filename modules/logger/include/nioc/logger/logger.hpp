////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdio>
#include <exception>
#include <nioc/common/annotators.hpp>
#include <spdlog/fmt/std.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

namespace nioc::logger
{

namespace detail
{

/// @brief Forward declaration of the logging core that the public severity functions call; the
/// contract is documented on its definition below.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept;

} // namespace detail

/// @brief The compile-time severity floor: the emit step of any log call below this level is
/// discarded at compile time and costs nothing at run time.
///
/// Set from `SPDLOG_ACTIVE_LEVEL`. Also the default run-time level for @ref setupDefaultLogger.
///
/// @see setupDefaultLogger
constexpr auto kDefaultActiveLevel = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

/// @brief The default log line layout: ISO-8601 timestamp, colored severity, `file:line`, then the
/// message.
///
/// In spdlog pattern syntax. Used by @ref setupDefaultLogger and @ref addSink unless you pass your
/// own pattern.
///
/// @see setupDefaultLogger, addSink
constexpr std::string_view kDefaultLogPattern = "[%Y-%m-%dT%H:%M:%S.%e%z] [%^%l%$] [%s:%#] %v";

/// @brief Log a `trace`-severity message through the process-wide default logger.
///
/// Example:
///
///     nioc::logger::trace("loaded {} items in {} ms", count, elapsed);
///
/// Never throws: a formatting or sink failure is written to stderr and swallowed. When `trace` is
/// below @ref kDefaultActiveLevel the emit is elided at compile time, but this is a plain function,
/// so @p args are still evaluated at the call site. Call @ref setupDefaultLogger first to control
/// where output goes; otherwise spdlog's stock logger is used.
///
/// @tparam Args Types of the values substituted into @p message.
///
/// @param message The format string. A string literal whose `{}` placeholders are filled by
/// @p args; it also captures the call-site file and line shown in the output.
///
/// @param args The values substituted into @p message, one per `{}` placeholder.
///
/// @see debug, info, warn, error, critical, setupDefaultLogger
template<typename... Args>
void trace(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::trace>(message, std::forward<Args>(args)...);
}

/// @brief Log a `debug`-severity message; otherwise identical to @ref trace.
///
/// @see trace
template<typename... Args>
void debug(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::debug>(message, std::forward<Args>(args)...);
}

/// @brief Log an `info`-severity message; otherwise identical to @ref trace.
///
/// @see trace
template<typename... Args>
void info(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::info>(message, std::forward<Args>(args)...);
}

/// @brief Log a `warn`-severity message; otherwise identical to @ref trace.
///
/// @see trace
template<typename... Args>
void warn(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::warn>(message, std::forward<Args>(args)...);
}

/// @brief Log an `error`-severity message; otherwise identical to @ref trace.
///
/// @see trace
template<typename... Args>
void error(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::err>(message, std::forward<Args>(args)...);
}

/// @brief Log a `critical`-severity message; otherwise identical to @ref trace.
///
/// @see trace
template<typename... Args>
void critical(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::critical>(message, std::forward<Args>(args)...);
}

/// @brief Install the process-wide default logger, backed by a fan-out sink that @ref addSink and
/// @ref removeSink later target.
///
/// Example:
///
///     nioc::logger::setupDefaultLogger("myApp");
///
/// Replaces any existing default logger. Call once at startup before any logging. Not safe to call
/// while other threads log.
///
/// @param name Tag identifying this logger.
///
/// @param enableStderrSink When true, attaches a colored stderr sink rendered with @p pattern.
///
/// @param logLevel Sets both the run-time severity threshold and the level at which the
/// logger flushes.
///
/// @param pattern spdlog layout for the stderr sink. Ignored when @p enableStderrSink is
/// false.
///
/// @see addSink, removeSink, kDefaultActiveLevel, kDefaultLogPattern
void setupDefaultLogger(
    std::string name,
    bool enableStderrSink = true,
    spdlog::level::level_enum logLevel = kDefaultActiveLevel,
    std::string_view pattern = kDefaultLogPattern);

/// @brief Attach a sink to the default logger so it also receives every log message.
///
/// When @ref setupDefaultLogger installed the fan-out, this attaches there and is synchronized, so
/// it is safe to call while other threads log. Otherwise it appends to spdlog's stock default
/// logger, which is not synchronized against concurrent logging. Shares ownership of @p sink.
///
/// @param sink The sink to attach. Its formatter is set to @p pattern.
///
/// @param pattern spdlog layout applied to @p sink. Set per sink because the fan-out does not push
/// its formatter to sinks added later.
///
/// @see removeSink, setupDefaultLogger
void addSink(spdlog::sink_ptr sink, std::string_view pattern = kDefaultLogPattern);

/// @brief Detach a previously attached sink from the default logger.
///
/// Detaching a sink that was never attached is a no-op. Mirrors @ref addSink's target selection:
/// synchronized, and safe under concurrent logging, only when the @ref setupDefaultLogger fan-out
/// is in place. Never throws; a lock failure is reported via @ref critical and swallowed.
///
/// @param sink The sink to detach.
///
/// @see addSink, setupDefaultLogger
void removeSink(const spdlog::sink_ptr& sink) noexcept;

namespace detail
{

/// @brief Format and emit a message at @p Level through the default logger; the shared core behind
/// @ref trace and the other severity functions.
///
/// The emit is elided at compile time when @p Level is below @ref kDefaultActiveLevel. Never
/// throws: any formatting or sink failure is written to stderr and swallowed. The spdlog pattern is
/// bypassed on this path, so the report carries its own fixed `[CRITICAL] [logger::logAt]` prefix
/// instead of the usual severity and source location.
///
/// @tparam Level Explicit template argument selecting the severity to log at.
///
/// @tparam Args Types of the values substituted into @p message.
///
/// @param message The format string, capturing the call-site file and line.
///
/// @param args The values substituted into @p message, one per `{}` placeholder.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  if constexpr(Level >= kDefaultActiveLevel)
  {
    // Logging must never throw: formatting and the sinks may, so a failure is reported to stderr
    // via fputs (itself no-throw) and swallowed rather than propagated into the caller. The normal
    // spdlog pattern that prepends severity and source location is gone on this path, so the
    // report carries them itself — a critical-severity failure originating in this function.
    try
    {
      spdlog::default_logger_raw()->log(
          spdlog::source_loc{
              message.mLocation.file_name(),
              static_cast<int>(message.mLocation.line()),
              message.mLocation.function_name()},
          Level,
          message.mFormat,
          std::forward<Args>(args)...);
    }
    catch(const std::exception& error)
    {
      static_cast<void>(std::fputs("[CRITICAL] [logger::logAt] ", stderr));
      static_cast<void>(std::fputs(error.what(), stderr));
      static_cast<void>(std::fputs("\n", stderr));
    }
    catch(...)
    {
      static_cast<void>(std::fputs("[CRITICAL] [logger::logAt] ", stderr));
      static_cast<void>(
          std::fputs("Caught unknown exception that is not derived from std::exception\n", stderr));
    }
  }
}

} // namespace detail

} // namespace nioc::logger
