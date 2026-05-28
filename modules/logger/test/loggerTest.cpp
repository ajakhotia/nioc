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

  nioc::logger::info("count={}", 42);

  // The timestamp is the only non-deterministic part; assert everything after it.
  EXPECT_EQ(buffer.str().substr(kTimestampPrefixWidth), "[info] count=42\n");
}

TEST(Logger, CapturesCallSiteSourceLocation)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "[%s] %v");

  nioc::logger::warn("here");

  EXPECT_EQ(buffer.str(), "[loggerTest.cpp] here\n");
}

TEST(Logger, EveryLevelFormatsAndEmits)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "[%l] %v");

  const auto lvalue = std::string{ "lvalue" };
  nioc::logger::trace("t {}", 1);
  nioc::logger::debug("d {} {}", 1, 2);
  nioc::logger::info("i {}", lvalue);
  nioc::logger::warn("w");
  nioc::logger::error("e {}", 3.14);
  nioc::logger::critical("c {}", lvalue);

  // Only levels at or above the compile-time threshold emit; build the expected output to match.
  using lvl = spdlog::level::level_enum;
  constexpr auto active = nioc::logger::kDefaultActiveLevel;
  auto expected = std::string{};
  if constexpr(lvl::trace >= active)
  {
    expected += "[trace] t 1\n";
  }
  if constexpr(lvl::debug >= active)
  {
    expected += "[debug] d 1 2\n";
  }
  if constexpr(lvl::info >= active)
  {
    expected += "[info] i lvalue\n";
  }
  if constexpr(lvl::warn >= active)
  {
    expected += "[warning] w\n";
  }
  if constexpr(lvl::err >= active)
  {
    expected += "[error] e 3.14\n";
  }
  if constexpr(lvl::critical >= active)
  {
    expected += "[critical] c lvalue\n";
  }

  EXPECT_EQ(buffer.str(), expected);
}

TEST(Logger, CompileTimeThresholdGatesLowerLevels)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "%v");

  // trace is the lowest severity. The expectation adapts to wherever kActiveLevel sits, so this
  // holds for whatever threshold the build system later selects.
  nioc::logger::trace("trace message");

  if constexpr(spdlog::level::trace >= nioc::logger::kDefaultActiveLevel)
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
  nioc::logger::setupDefaultLogger("loggerTest");

  auto buffer = std::ostringstream{};
  const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(buffer);
  nioc::logger::addSink(sink);

  nioc::logger::info("hello {}", 1);
  EXPECT_NE(buffer.str().find("hello 1"), std::string::npos);

  nioc::logger::removeSink(sink);
  buffer.str("");
  nioc::logger::info("gone {}", 2);
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
  nioc::logger::addSink(sink);

  nioc::logger::info("hello {}", 1);
  EXPECT_NE(extra.str().find("hello 1"), std::string::npos);

  nioc::logger::removeSink(sink);
  extra.str("");
  nioc::logger::info("gone {}", 2);
  EXPECT_TRUE(extra.str().empty());
}

} // namespace nioc::logger
