////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <nioc/logger/logger.hpp>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace nioc::logger
{
namespace
{

/// A sink that fails on every message; exercises the never-throws contract of the level functions.
class ThrowingSink final: public spdlog::sinks::base_sink<std::mutex>
{
private:
  void sink_it_(const spdlog::details::log_msg& /*msg*/) override
  {
    throw std::runtime_error{"sink failure"};
  }

  void flush_() override {}
};

/// A sink that fails with a value not derived from std::exception; exercises the catch-all path.
class IntThrowingSink final: public spdlog::sinks::base_sink<std::mutex>
{
private:
  void sink_it_(const spdlog::details::log_msg& /*msg*/) override
  {
    constexpr auto kArbitraryValue = 42;
    // NOLINTNEXTLINE(hicpp-exception-baseclass): the non-std exception path is the test subject
    throw int{kArbitraryValue};
  }

  void flush_() override {}
};

// spdlog renders the ISO 8601 timestamp "%Y-%m-%dT%H:%M:%S.%eZ" as a fixed-width field, e.g.
// "2026-05-27T14:02:11.337Z". With the surrounding brackets and trailing space the prefix is
// always this many characters, so a test can skip the timestamp and assert the remainder exactly.
constexpr auto kTimestampPrefixWidth = std::string_view{"[2026-05-27T14:02:11.337Z] "}.size();

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

  constexpr auto kCount = 42;
  info("count={}", kCount);

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

  constexpr auto kErrorValue = 3.14;
  const auto lvalue = std::string{"lvalue"};
  trace("t {}", 1);
  debug("d {} {}", 1, 2);
  info("i {}", lvalue);
  warn("w");
  error("e {}", kErrorValue);
  critical("c {}", lvalue);

  // Only levels at or above the compile-time threshold emit; build the expected output to match.
  using LevelEnum = spdlog::level::level_enum;
  auto expected = std::string{};

  expected += LevelEnum::trace >= kDefaultActiveLevel ? "[trace] t 1\n" : "";
  expected += LevelEnum::debug >= kDefaultActiveLevel ? "[debug] d 1 2\n" : "";
  expected += LevelEnum::info >= kDefaultActiveLevel ? "[info] i lvalue\n" : "";
  expected += LevelEnum::warn >= kDefaultActiveLevel ? "[warning] w\n" : "";
  expected += LevelEnum::err >= kDefaultActiveLevel ? "[error] e 3.14\n" : "";
  expected += LevelEnum::critical >= kDefaultActiveLevel ? "[critical] c lvalue\n" : "";

  EXPECT_EQ(buffer.str(), expected);
}

TEST(Logger, CompileTimeThresholdGatesLowerLevels)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "%v");

  // trace is the lowest severity. The expectation adapts to wherever kDefaultActiveLevel
  // sits, so this holds for whatever threshold the build selects.
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

TEST(Logger, SinkFailureDoesNotPropagateToCaller)
{
  setupDefaultLogger("loggerTest", false);

  const auto sink = std::make_shared<ThrowingSink>();
  addSink(sink);

  EXPECT_NO_THROW(info("message into a failing sink"));

  removeSink(sink);
}

TEST(Logger, NonStdExceptionFromSinkDoesNotPropagateToCaller)
{
  setupDefaultLogger("loggerTest", false);

  const auto sink = std::make_shared<IntThrowingSink>();
  addSink(sink);

  EXPECT_NO_THROW(info("message into a sink throwing a non-std exception"));

  removeSink(sink);
}

TEST(Logger, ArgumentEvaluationIsNotElidedBelowTheThreshold)
{
  auto buffer = std::ostringstream{};
  installBufferLogger(buffer, "%v");

  // The level functions are plain functions, not macros: an argument with a side effect runs at
  // the call site even when the call itself compiles to nothing.
  auto evaluations = 0;
  const auto evaluate = [&evaluations]
  {
    ++evaluations;
    return 1;
  };

  trace("value {}", evaluate());
  EXPECT_EQ(1, evaluations);
}

TEST(LoggerSetup, RuntimeLevelDropsMessagesBelowIt)
{
  setupDefaultLogger("loggerTest", false, spdlog::level::warn);

  auto buffer = std::ostringstream{};
  const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(buffer);
  addSink(sink);

  info("dropped");
  warn("kept");

  EXPECT_EQ(buffer.str().find("dropped"), std::string::npos);
  EXPECT_NE(buffer.str().find("kept"), std::string::npos);

  removeSink(sink);
}

TEST(LoggerSetup, AddSinkAppliesThePatternToTheSink)
{
  setupDefaultLogger("loggerTest", false);

  auto buffer = std::ostringstream{};
  const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(buffer);
  addSink(sink, "%v");

  info("bare {}", 1);

  // The fan-out does not propagate a formatter to later-added sinks; addSink formats the sink
  // itself, so the output carries exactly the requested pattern.
  EXPECT_EQ("bare 1\n", buffer.str());

  removeSink(sink);
}

TEST(LoggerSetup, RemovingAbsentSinkIsANoOp)
{
  setupDefaultLogger("loggerTest", false);

  const auto neverAdded = std::make_shared<ThrowingSink>();
  EXPECT_NO_THROW(removeSink(neverAdded));
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
