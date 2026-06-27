////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <csignal>
#include <cstdint>
#include <gtest/gtest.h>
#include <nioc/common/signalCatcher.hpp>
#include <thread>
#include <utility>
#include <vector>

namespace nioc::common
{
namespace
{

using namespace std::chrono_literals;

// The watch thread drains a single-slot atomic cache, so raising a second signal before that
// slot is consumed would clobber the first. Sleeping after each raise lets the watcher catch up.
constexpr auto kDrainDelay = 500us;

TEST(SignalCatcher, CatchesSigintAndSigabrt)
{
  auto sigintCount = std::int32_t{0};
  auto sigabrtCount = std::int32_t{0};

  const auto catcher = SignalCatcher{
      std::pair{
                SIGINT,   SignalCatcher::SignalAction{[&sigintCount](std::int32_t) { ++sigintCount; }} },
      std::pair{
                SIGABRT, SignalCatcher::SignalAction{[&sigabrtCount](std::int32_t) { ++sigabrtCount; }}}
  };

  static_cast<void>(std::raise(SIGINT));
  std::this_thread::sleep_for(kDrainDelay);

  static_cast<void>(std::raise(SIGABRT));
  std::this_thread::sleep_for(kDrainDelay);

  EXPECT_EQ(1, sigintCount);
  EXPECT_EQ(1, sigabrtCount);
}

TEST(SignalCatcher, ActionReceivesRunningCount)
{
  auto observedCounts = std::vector<std::int32_t>{};

  const auto catcher = SignalCatcher{
      std::pair{
                SIGINT, SignalCatcher::SignalAction{[&observedCounts](const std::int32_t count)
                                      { observedCounts.push_back(count); }}}
  };

  static_cast<void>(std::raise(SIGINT));
  std::this_thread::sleep_for(kDrainDelay);

  static_cast<void>(std::raise(SIGINT));
  std::this_thread::sleep_for(kDrainDelay);

  EXPECT_EQ((std::vector{1, 2}), observedCounts);
}

TEST(SignalCatcher, DestructionRestoresDefaultHandlers)
{
  {
    const auto catcher = SignalCatcher{
        std::pair{SIGINT, SignalCatcher::SignalAction{[](std::int32_t) {}}}
    };
  }

  // The destructor restored SIG_DFL; std::signal hands back the handler in effect.
  const auto previousHandler = std::signal(SIGINT, SIG_DFL);
  EXPECT_EQ(SIG_DFL, previousHandler);
}

} // namespace
} // namespace nioc::common
