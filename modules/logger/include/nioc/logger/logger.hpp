////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nioc/common/annotators.hpp>
#include <spdlog/fmt/std.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace nioc::logger
{

namespace detail
{

// logAt is the logging-specific machinery, forward declared here so the public API and its
// documentation come first; it is defined at the bottom of this file. The format-string wrapper it
// takes is the shared common::FormatWithLocation.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(const common::FormatWithLocation& message, Args&&... args);

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
///
/// - Default runtime level: @ref setupDefaultLogger uses it as the default for its logLevel
///   parameter.
constexpr auto kDefaultActiveLevel = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

/// @brief Logs a message at trace severity to the default logger.
///
/// The most detailed level: fine-grained tracing, usually turned on only while diagnosing a
/// specific problem. The file and line of the call are recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::trace("packet {}: byte {} of {}", packetId, offset, total);
/// @endcode
template<typename... Args>
void trace(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::trace>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at debug severity to the default logger.
///
/// Diagnostic detail is useful while developing; typically disabled in production. The file and
/// line of the call are recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::debug("cache hit for key '{}'", key);
/// @endcode
template<typename... Args>
void debug(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::debug>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at info severity to the default logger.
///
/// Normal operational events that are worth recording: startup, configuration, lifecycle
/// milestones. The file and line of the call are recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::info("listening on port {}", port);
/// @endcode
template<typename... Args>
void info(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::info>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at warning severity to the default logger.
///
/// An unexpected but recoverable situation worth flagging. The file and line of the call are
/// recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::warn("dropped {} stale frames", droppedCount);
/// @endcode
template<typename... Args>
void warn(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::warn>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at error severity to the default logger.
///
/// An operation failed, but the program continues running. The file and line of the call are
/// recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::error("failed to open {}: {}", path.string(), ec.message());
/// @endcode
template<typename... Args>
void error(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::err>(message, std::forward<Args>(args)...);
}

/// @brief Logs a message at critical severity to the default logger.
///
/// A fatal condition, typically recorded just before the program aborts. The file and line of
/// the call are recorded automatically.
///
/// @param message Format string with `{}` placeholders (spdlog/fmt syntax).
///
/// @param args Values substituted into the placeholders, left to right.
///
/// @code
/// nioc::logger::critical("checksum mismatch on {}, aborting", path.string());
/// @endcode
template<typename... Args>
void critical(const common::FormatWithLocation& message, Args&&... args)
{
  detail::logAt<spdlog::level::critical>(message, std::forward<Args>(args)...);
}

/// @brief Installs the process-wide default logger: colored console output with an ISO 8601 UTC
/// timestamp, severity, and source location.
///
/// Call once at program start, before any logging. The logger writes through a sink list that
/// @ref addSink can extend at run time — for example, a terminus Port adding a file sink so the
/// run's console output is also captured in its recording.
///
/// @param name Name assigned to the logger; identifies it in spdlog's registry and appears in
/// output when the pattern includes the `%n` flag.
///
/// @param logLevel Runtime minimum severity; messages below it are dropped. Defaults to
/// kDefaultActiveLevel. A value below the compile-time floor has no effect, since those calls are
/// already elided.
void setupDefaultLogger(std::string name, spdlog::level::level_enum logLevel = kDefaultActiveLevel);

/// @brief Adds @p sink to the default logger, where it receives every subsequent message until
/// detached with @ref removeSink.
///
/// If @ref setupDefaultLogger installed the nioc fan-out, the sink is attached there — that path
/// is synchronized, so it is safe to call while other threads log. Otherwise the sink is attached
/// directly to spdlog's own default logger as a best-effort fallback; that fallback path is not
/// synchronized against concurrent logging.
///
/// @param sink Sink to attach.
void addSink(spdlog::sink_ptr sink);

/// @brief Removes a sink previously attached with @ref addSink; does nothing if the @p sink is
/// absent.
///
/// @param sink Sink to detach.
void removeSink(const spdlog::sink_ptr& sink);

namespace detail
{

/// @brief Forwards a formatted message to the process-wide default logger.
///
/// Centralizes the source-location conversion and the compile-time level gate so every level
/// shares one implementation. When @p Level is below nioc::logger::kDefaultActiveLevel the body is
/// discarded and the call compiles to nothing. Otherwise, spdlog still formats lazily, skipping
/// the work when the logger's runtime level is higher than @p Level.
///
/// @tparam Level Severity the message is logged at.
template<spdlog::level::level_enum Level, typename... Args>
void logAt(const common::FormatWithLocation& message, Args&&... args)
{
  if constexpr(Level >= kDefaultActiveLevel)
  {
    spdlog::default_logger_raw()->log(
        spdlog::source_loc{ message.mLocation.file_name(),
                            static_cast<int>(message.mLocation.line()),
                            message.mLocation.function_name() },
        Level,
        fmt::runtime(message.mFormat),
        std::forward<Args>(args)...);
  }
}

} // namespace detail

} // namespace nioc::logger
