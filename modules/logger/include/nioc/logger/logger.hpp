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

template<spdlog::level::level_enum Level, typename... Args>
void logAt(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept;

} // namespace detail

/// @brief Default severity: the compile-time minimum and the default runtime level.
///
/// Comes from spdlog's SPDLOG_ACTIVE_LEVEL (default SPDLOG_LEVEL_INFO). Set it at build time the
/// spdlog way: `-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN`. Calls below this level compile to
/// nothing. The runtime level can raise the minimum but not lower it below this. Call arguments
/// still get evaluated, so avoid log arguments with side effects.
constexpr auto kDefaultActiveLevel = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

/// @brief Default format pattern applied to each sink this API attaches.
///
/// In spdlog/fmt pattern syntax. Shows an ISO 8601 local timestamp with the local UTC offset
/// (`%z`), colored severity, source location, and the message. The offset makes each line state its
/// own zone, so no fixed zone is assumed.
constexpr std::string_view kDefaultLogPattern = "[%Y-%m-%dT%H:%M:%S.%e%z] [%^%l%$] [%s:%#] %v";

/// @brief Logs at trace severity: fine detail, usually on only while diagnosing a problem.
///
/// Captures the call site. Never throws; a failure goes to stderr. The format string is checked
/// against @p args at compile time: a placeholder/argument mismatch fails to compile.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values for the placeholders, left to right.
///
/// @code
/// nioc::logger::trace("packet {}: byte {} of {}", packetId, offset, total);
/// @endcode
template<typename... Args>
void trace(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::trace>(message, std::forward<Args>(args)...);
}

/// @brief Logs at debug severity: diagnostic detail useful while developing.
///
/// Same contract as @ref trace.
template<typename... Args>
void debug(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::debug>(message, std::forward<Args>(args)...);
}

/// @brief Logs at info severity: normal events worth recording, such as startup, configuration,
/// and lifecycle milestones.
///
/// Same contract as @ref trace.
template<typename... Args>
void info(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::info>(message, std::forward<Args>(args)...);
}

/// @brief Logs at warning severity: an unexpected but recoverable situation worth flagging.
///
/// Same contract as @ref trace.
template<typename... Args>
void warn(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::warn>(message, std::forward<Args>(args)...);
}

/// @brief Logs at error severity: an operation failed but the program keeps running.
///
/// Same contract as @ref trace.
template<typename... Args>
void error(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::err>(message, std::forward<Args>(args)...);
}

/// @brief Logs at critical severity: a fatal condition, usually logged just before the program
/// aborts.
///
/// Same contract as @ref trace.
template<typename... Args>
void critical(
    const common::FormatWithLocation<spdlog::format_string_t<Args...>>& message,
    Args&&... args) noexcept
{
  detail::logAt<spdlog::level::critical>(message, std::forward<Args>(args)...);
}

/// @brief Installs the process-wide default logger.
///
/// Call once at program start, before any logging. @ref addSink can add more sinks at run time.
///
/// @param name Logger name. Identifies it in spdlog's registry and appears in output when the
/// pattern includes the `%n` flag.
///
/// @param enableStderrSink True (the default) attaches a colored stderr sink. False installs the
/// logger with no stderr sink.
///
/// @param logLevel Runtime minimum severity; messages below it are dropped. Defaults to
/// kDefaultActiveLevel. A value below the compile-time minimum has no effect, since those calls are
/// already removed.
///
/// @param pattern Format pattern (spdlog syntax) for the stderr sink. Defaults to
/// kDefaultLogPattern. Ignored when @p enableStderrSink is false.
void setupDefaultLogger(
    std::string name,
    bool enableStderrSink = true,
    spdlog::level::level_enum logLevel = kDefaultActiveLevel,
    std::string_view pattern = kDefaultLogPattern);

/// @brief Adds @p sink to the default logger; it receives every later message until removed with
/// @ref removeSink.
///
/// Thread-safe if @ref setupDefaultLogger was called first: the sink attaches to the nioc fan-out,
/// which is synchronized, so you may call this while other threads log. Without that setup, the
/// sink attaches to spdlog's own default logger, which is not synchronized against concurrent
/// logging.
///
/// @param sink Sink to attach.
///
/// @param pattern Format pattern (spdlog syntax) applied to @p sink. Defaults to
/// kDefaultLogPattern, matching the stderr sink from @ref setupDefaultLogger.
void addSink(spdlog::sink_ptr sink, std::string_view pattern = kDefaultLogPattern);

/// @brief Removes a sink added with @ref addSink. Does nothing if @p sink is not attached.
///
/// Never throws, so it is safe to call from destructors and other teardown paths. On an internal
/// failure the sink stays attached and the failure is logged at critical.
///
/// @param sink Sink to detach.
void removeSink(const spdlog::sink_ptr& sink) noexcept;

namespace detail
{

/// @brief Forwards a formatted message to the process-wide default logger.
///
/// Shared implementation behind every level function. When @p Level is below
/// nioc::logger::kDefaultActiveLevel the call compiles to nothing. Otherwise spdlog skips the
/// formatting work at run time when the logger's runtime level is above @p Level.
///
/// Never throws: any exception from formatting or the sinks is caught and written to stderr instead
/// of reaching the caller.
///
/// @tparam Level Severity the message is logged at.
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
