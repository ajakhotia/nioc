////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <memory>
#include <nioc/logger/logger.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <string>
#include <string_view>

namespace nioc::logger
{
namespace
{

// spdlog renders the ISO 8601 timestamp "%Y-%m-%dT%H:%M:%S.%eZ" as a fixed-width field, e.g.
// "2026-05-27T14:02:11.337Z". With the surrounding brackets and trailing space the prefix is
// always this many characters, so a test can skip the timestamp and assert the remainder exactly.
constexpr auto kTimestampPrefixWidth = std::string_view{ "[2026-05-27T14:02:11.337Z] " }.size();

/// Installs a default logger that writes to @p buffer using @p pattern.
void installBufferLogger(std::ostringstream& buffer, std::string pattern)
{
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(buffer);
  auto logger = std::make_shared<spdlog::logger>("loggerTest", std::move(sink));
  logger->set_pattern(std::move(pattern), spdlog::pattern_time_type::utc);
  logger->set_level(spdlog::level::trace);
  spdlog::set_default_logger(std::move(logger));
}

} // namespace

TEST(Logger, FormatsArgumentsIntoMessage)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "[%Y-%m-%dT%H:%M:%S.%eZ] [%l] %v");

  info("count={}", 42);

  // The timestamp is the only non-deterministic part; assert everything after it.
  EXPECT_EQ(buffer.str().substr(kTimestampPrefixWidth), "[info] count=42\n");
}

TEST(Logger, CapturesCallSiteSourceLocation)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "[%s] %v");

  warn("here");

  EXPECT_EQ(buffer.str(), "[loggerTest.cpp] here\n");
}

TEST(Logger, EveryLevelFormatsAndEmits)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "[%l] %v");

  const auto lvalue = std::string{ "lvalue" };
  trace("t {}", 1);
  debug("d {} {}", 1, 2);
  info("i {}", lvalue);
  warn("w");
  error("e {}", 3.14);
  critical("c {}", lvalue);

  // Only levels at or above the compile-time threshold emit; build the expected output to match.
  using level_enum = spdlog::level::level_enum;
  auto expected = std::string{};

  expected += level_enum::trace >= kDefaultActiveLevel ? "[trace] t 1\n" : "";
  expected += level_enum::debug >= kDefaultActiveLevel ? "[debug] d 1 2\n" : "";
  expected += level_enum::info >= kDefaultActiveLevel ? "[info] i lvalue\n" : "";
  expected += level_enum::warn >= kDefaultActiveLevel ? "[warning] w\n" : "";
  expected += level_enum::err >= kDefaultActiveLevel ? "[error] e 3.14\n" : "";
  expected += level_enum::critical >= kDefaultActiveLevel ? "[critical] c lvalue\n" : "";

  EXPECT_EQ(buffer.str(), expected);
}

TEST(Logger, CompileTimeThresholdGatesLowerLevels)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "%v");

  // trace is the lowest severity. The expectation adapts to wherever kkDefaultActiveLevelLevel
  // sits, so this holds for whatever threshold the build system later selects.
  trace("trace message");

  if constexpr(spdlog::level::trace >= kDefaultActiveLevel)
  {
    EXPECT_EQ(buffer.str(), "trace message\n");
  }
  else
  {
    EXPECT_TRUE(buffer.str().empty());
  }
}

TEST(LoggerSetup, AddedSinkReceivesMessagesUntilRemoved)
{
  setupDefaultLogger("loggerTest", false);

  auto buffer = std::ostringstream{};
  const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(buffer);
  addSink(sink);

  info("hello {}", 1);
  EXPECT_NE(buffer.str().find("hello 1"), std::string::npos);

  removeSink(sink);
  buffer.str("");
  info("gone {}", 2);
  EXPECT_TRUE(buffer.str().empty());
}

TEST(LoggerSetup, AddSinkFallsBackToExistingLoggerWithoutSetup)
{
  // No setupDefaultLogger here: a plain logger stands in for spdlog's own default. addSink must
  // attach to it rather than throw.
  auto base = std::ostringstream{};
  installBufferLogger(base, "%v");

  auto extra = std::ostringstream{};
  const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(extra);
  addSink(sink);

  info("hello {}", 1);
  EXPECT_NE(extra.str().find("hello 1"), std::string::npos);

  removeSink(sink);
  extra.str("");
  info("gone {}", 2);
  EXPECT_TRUE(extra.str().empty());
}

} // namespace nioc::logger
