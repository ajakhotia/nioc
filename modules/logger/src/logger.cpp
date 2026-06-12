////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <nioc/logger/logger.hpp>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace nioc::logger
{

namespace
{

// Returns the dist_sink that fans out the default logger's output, or nullptr when the default
// logger was not installed by setupDefaultLogger. Never throws: reading the registry locks a mutex
// whose failure is unrecoverable, so it is logged and surfaced as an absent fan-out. critical is
// itself noexcept, so the report cannot trigger a second failure.
std::shared_ptr<spdlog::sinks::dist_sink_mt> defaultFanOut() noexcept
{
  try
  {
    if(const auto logger = spdlog::default_logger(); logger and not logger->sinks().empty())
    {
      return std::dynamic_pointer_cast<spdlog::sinks::dist_sink_mt>(logger->sinks().front());
    }
  }
  catch(const std::system_error& error)
  {
    critical("Failed to access the default logger: {}", error.what());
  }

  return nullptr;
}

} // namespace

void setupDefaultLogger(
    std::string name,
    const bool enableStderrSink,
    const spdlog::level::level_enum logLevel,
    const std::string_view pattern)
{
  // Install the default logger over an initially empty fan-out and apply the run-time spec.
  auto fanOut = std::make_shared<spdlog::sinks::dist_sink_mt>();
  auto logger = std::make_shared<spdlog::logger>(std::move(name), fanOut);

  logger->set_level(logLevel);
  logger->flush_on(logLevel);

  spdlog::set_default_logger(std::move(logger));

  // Attach the colored stderr sink unless suppressed.
  if(enableStderrSink)
  {
    auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    sink->set_pattern(std::string{pattern});

    fanOut->add_sink(std::move(sink));
  }
}

void addSink(spdlog::sink_ptr sink, const std::string_view pattern)
{
  // Set the pattern per sink: dist_sink does not carry its formatter to sinks added later, so a
  // sink attached here would otherwise render with spdlog's default pattern instead of ours.
  sink->set_pattern(std::string{pattern});

  // When setupDefaultLogger installed our fan-out, attach there: add_sink on a dist_sink is
  // synchronized, so sinks can be added while other threads log.
  if(const auto fanOut = defaultFanOut())
  {
    fanOut->add_sink(std::move(sink));
    return;
  }

  // No nioc fan-out is present (setupDefaultLogger was not called). spdlog always provides a
  // default logger, so attach to it directly. Best-effort: mutating a plain logger's sink list is
  // not synchronized against concurrent logging.
  spdlog::default_logger_raw()->sinks().push_back(std::move(sink));
}

void removeSink(const spdlog::sink_ptr& sink) noexcept
{
  // Mirror addSink: detach from the fan-out when present, otherwise from the plain default
  // logger's sink list. Either way, an absent sink is a no-op. remove_sink locks the fan-out's
  // mutex, the only operation here that can throw; honor the noexcept contract by reporting a lock
  // failure rather than letting it escape.
  try
  {
    if(const auto fanOut = defaultFanOut())
    {
      fanOut->remove_sink(sink);
      return;
    }

    std::erase(spdlog::default_logger_raw()->sinks(), sink);
  }
  catch(const std::system_error& error)
  {
    critical("Failed to detach sink: {}", error.what());
  }
}

} // namespace nioc::logger
