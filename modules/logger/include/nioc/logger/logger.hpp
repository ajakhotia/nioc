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

// logAt is the logging-specific machinery, forward declared here so the public API and its
// documentation come first; it is defined at the bottom of this file. The format-string wrapper it
// takes is the shared common::FormatWithLocation.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(const common::FormatWithLocation& message, Args&&... args) noexcept;

} // namespace detail

/// @brief The default active severity: both the compile-time floor and the default runtime level.
///
/// Derived from spdlog's own SPDLOG_ACTIVE_LEVEL (default SPDLOG_LEVEL_INFO), so the build sets it
/// the standard spdlog way — `-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN`, etc. — and both spdlog's
/// macros and these functions honor the same threshold. It serves two roles:
///
/// - Compile-time floor: in the level functions, a call below this severity expands to an empty
///   body and is inlined away, eliding the logging call and the formatting — but not the
///   evaluation of the arguments at the call site, which a plain function cannot suppress the way
///   a macro can, so avoid log arguments with side effects. This floor is hard: the runtime level
///   can narrow above it but cannot reach below it.
constexpr auto kDefaultActiveLevel = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

/// @brief Default formatting pattern applied to each sink the nioc logging API attaches.
///
/// An ISO 8601 local timestamp with a numeric UTC offset, colored severity, source location, and
/// the message — in spdlog/fmt pattern-flag syntax. The `%z` offset is rendered from local time, so
/// the timestamp is self-describing without asserting a fixed zone.
constexpr std::string_view kDefaultLogPattern = "[%Y-%m-%dT%H:%M:%S.%e%z] [%^%l%$] [%s:%#] %v";

/// @brief Logs a message at trace severity to the default logger.
///
/// The most detailed level: fine-grained tracing, usually turned on only while diagnosing a
/// specific problem. The file and line of the call are recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::trace("packet {}: byte {} of {}", packetId, offset, total);
/// @endcode
template<typename... Args>
void trace(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::trace>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at debug severity to the default logger.
///
/// Diagnostic detail is useful while developing; typically disabled in production. The file and
/// line of the call are recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::debug("cache hit for key '{}'", key);
/// @endcode
template<typename... Args>
void debug(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::debug>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at info severity to the default logger.
///
/// Normal operational events that are worth recording: startup, configuration, lifecycle
/// milestones. The file and line of the call are recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::info("listening on port {}", port);
/// @endcode
template<typename... Args>
void info(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::info>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at warning severity to the default logger.
///
/// An unexpected but recoverable situation worth flagging. The file and line of the call are
/// recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::warn("dropped {} stale frames", droppedCount);
/// @endcode
template<typename... Args>
void warn(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::warn>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at error severity to the default logger.
///
/// An operation failed, but the program continues running. The file and line of the call are
/// recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::error("failed to open {}: {}", path.string(), ec.message());
/// @endcode
template<typename... Args>
void error(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::err>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at critical severity to the default logger.
///
/// A fatal condition, typically recorded just before the program aborts. The file and line of
/// the call are recorded automatically.
///
/// Never throws: a logging failure is written to stderr and otherwise ignored.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::critical("checksum mismatch on {}, aborting", path.string());
/// @endcode
template<typename... Args>
void critical(const common::FormatWithLocation& message, Args&&... args) noexcept
{
  detail::logAt<spdlog::level::critical>(message, std::forward<Args>(args)...);
}

/// @brief Installs the process-wide default logger, optionally with a colored stderr sink whose
/// output carries an ISO 8601 local timestamp with a UTC offset, severity, and source location.
///
/// Call once at program start, before any logging. The logger writes through a sink list that
/// @ref addSink can extend at run time.
///
/// @param name Name assigned to the logger; identifies it in spdlog's registry and appears in
/// output when the pattern includes the `%n` flag.
///
/// @param logLevel Runtime minimum severity; messages below it are dropped. Defaults to
/// kDefaultActiveLevel. A value below the compile-time floor has no effect, since those calls are
/// already elided.
///
/// @param enableStderrSink When true (the default), a colored stderr sink is attached. Pass false
/// to install the logger without the stderr console-sink.
///
/// @param pattern Formatting pattern (spdlog flag syntax) applied to the stderr sink. Defaults to
/// kDefaultLogPattern. Ignored when @p enableStderrSink is false.
void setupDefaultLogger(
    std::string name,
    bool enableStderrSink = true,
    spdlog::level::level_enum logLevel = kDefaultActiveLevel,
    std::string_view pattern = kDefaultLogPattern);

/// @brief Adds @p sink to the default logger, where it receives every subsequent message until
/// detached with @ref removeSink.
///
/// If @ref setupDefaultLogger installed the nioc fan-out, the sink is attached there — that path
/// is synchronized, so it is safe to call while other threads log. Otherwise, the sink is attached
/// directly to spdlog's own default logger as a best-effort fallback; that fallback path is not
/// synchronized against concurrent logging.
///
/// @param sink Sink to attach.
///
/// @param pattern Formatting pattern (spdlog flag syntax) applied to @p sink before it is attached.
/// Defaults to kDefaultLogPattern, so sinks added this way match the stderr sink installed by
/// @ref setupDefaultLogger.
void addSink(spdlog::sink_ptr sink, std::string_view pattern = kDefaultLogPattern);

/// @brief Removes a sink previously attached with @ref addSink; does nothing if the @p sink is
/// absent.
///
/// Never throws, so it is safe to call from teardown paths such as destructors. Detaching a sink
/// allocates nothing; should the logger's internal mutex fail — an unrecoverable condition — the
/// failure is logged at critical and the sink is left attached.
///
/// @param sink Sink to detach.
void removeSink(const spdlog::sink_ptr& sink) noexcept;

namespace detail
{

/// @brief Forwards a formatted message to the process-wide default logger.
///
/// Centralizes the source-location conversion and the compile-time level gate so every level
/// shares one implementation. When @p Level is below nioc::logger::kDefaultActiveLevel the body is
/// discarded and the call compiles to nothing. Otherwise, spdlog still formats lazily, skipping
/// the work when the logger's runtime level is higher than @p Level.
///
/// Never throws: any exception from formatting or the sinks is caught and written to stderr, so a
/// logging failure cannot propagate into the caller.
///
/// @tparam Level Severity the message is logged at.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(const common::FormatWithLocation& message, Args&&... args) noexcept
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
          fmt::runtime(message.mFormat),
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
