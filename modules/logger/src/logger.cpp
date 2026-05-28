////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/logger/logger.hpp>

#include <memory>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

namespace nioc::logger
{

namespace
{

// Returns the dist_sink that fans out the default logger's output, or nullptr when the default
// logger was not installed by setupDefaultLogger.
std::shared_ptr<spdlog::sinks::dist_sink_mt> defaultFanOut()
{
  if(const auto logger = spdlog::default_logger(); logger and not logger->sinks().empty())
  {
    return std::dynamic_pointer_cast<spdlog::sinks::dist_sink_mt>(logger->sinks().front());
  }
  return nullptr;
}

} // namespace

void setupDefaultLogger(std::string name, const spdlog::level::level_enum logLevel)
{
  auto fanOut = std::make_shared<spdlog::sinks::dist_sink_mt>();
  fanOut->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());

  auto logger = std::make_shared<spdlog::logger>(std::move(name), std::move(fanOut));

  // ISO 8601 UTC timestamp, colored severity, source location, message. The UTC time type makes
  // the trailing 'Z' truthful — the spdlog-flag counterpart of common::kIso8601UtcFormat.
  logger->set_pattern(
      "[%Y-%m-%dT%H:%M:%S.%eZ] [%^%l%$] [%s:%#] %v",
      spdlog::pattern_time_type::utc);

  logger->set_level(logLevel);
  logger->flush_on(logLevel);

  spdlog::set_default_logger(std::move(logger));
}

void addSink(spdlog::sink_ptr sink)
{
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

void removeSink(const spdlog::sink_ptr& sink)
{
  // Mirror addSink: detach from the fan-out when present, otherwise from the plain default
  // logger's sink list. Either way, an absent sink is a no-op.
  if(const auto fanOut = defaultFanOut())
  {
    fanOut->remove_sink(sink);
    return;
  }

  std::erase(spdlog::default_logger_raw()->sinks(), sink);
}

} // namespace nioc::logger
