////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/circular_buffer.hpp>
#include <deque>
#include <gtest/gtest.h>
#include <nioc/common/raiiToken.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nioc::common
{
namespace
{

TEST(RaiiToken, EntryRunsAtConstructionExitRunsAtDestruction)
{
  auto entryCount = 0;
  auto exitCount = 0;
  {
    const auto token = RaiiToken{
        [&entryCount] { ++entryCount; },
        [&exitCount]() noexcept { ++exitCount; }};

    EXPECT_EQ(1, entryCount);
    EXPECT_EQ(0, exitCount);
  }

  EXPECT_EQ(1, entryCount);
  EXPECT_EQ(1, exitCount);
}

TEST(RaiiToken, EntryReceivesForwardedArguments)
{
  auto sum = 0;
  {
    const auto token = RaiiToken{
        [&sum](const int lhs, const int rhs) { sum = lhs + rhs; },
        [&sum]() noexcept { --sum; },
        3,
        4};

    EXPECT_EQ(7, sum);
  }

  EXPECT_EQ(6, sum);
}

TEST(RaiiToken, ExitDoesNotRunWhenEntryThrows)
{
  auto exitCount = 0;

  const auto buildTokenWithThrowingEntry = [&exitCount]
  {
    const auto token = RaiiToken{
        [] { throw std::runtime_error{"entry failed"}; },
        [&exitCount]() noexcept { ++exitCount; }};
  };

  EXPECT_THROW(buildTokenWithThrowingEntry(), std::runtime_error);
  EXPECT_EQ(0, exitCount);
}

// Builds tokens that all share one exit-action type: entry bumps a live counter, exit drops it.
// A balanced count back to zero proves every exit ran exactly once across container relocation.
auto makeCountingToken(int& liveCount)
{
  return RaiiToken{[&liveCount] { ++liveCount; }, [&liveCount]() noexcept { --liveCount; }};
}

using CountingToken = decltype(makeCountingToken(std::declval<int&>()));

TEST(RaiiToken, VectorPushPopRunsExitExactlyOnce)
{
  auto liveCount = 0;
  {
    auto tokens = std::vector<CountingToken>{};
    tokens.push_back(makeCountingToken(liveCount)); // growth move-constructs; husks stay disengaged
    tokens.push_back(makeCountingToken(liveCount));
    tokens.push_back(makeCountingToken(liveCount));
    EXPECT_EQ(3, liveCount);

    tokens.erase(tokens.begin()); // pop front: shifts survivors down via move-assignment
    EXPECT_EQ(2, liveCount);

    tokens.push_back(makeCountingToken(liveCount));
    EXPECT_EQ(3, liveCount);
  }

  EXPECT_EQ(0, liveCount);
}

TEST(RaiiToken, CircularBufferOverwriteRunsExitExactlyOnce)
{
  auto liveCount = 0;
  {
    auto tokens = boost::circular_buffer<CountingToken>{2};
    tokens.push_back(makeCountingToken(liveCount));
    tokens.push_back(makeCountingToken(liveCount));
    EXPECT_EQ(2, liveCount);

    tokens.push_back(
        makeCountingToken(liveCount)); // full: overwrites the oldest via move-assignment
    EXPECT_EQ(2, liveCount);           // the evicted token's exit ran exactly once

    tokens.pop_front();
    EXPECT_EQ(1, liveCount);
  }

  EXPECT_EQ(0, liveCount);
}

TEST(RaiiToken, DequePushPopRunsExitExactlyOnce)
{
  auto liveCount = 0;
  {
    auto tokens = std::deque<CountingToken>{};
    tokens.push_back(makeCountingToken(liveCount));
    tokens.push_back(makeCountingToken(liveCount));
    EXPECT_EQ(2, liveCount);

    tokens.pop_front();
    EXPECT_EQ(1, liveCount);

    tokens.push_back(makeCountingToken(liveCount));
    tokens.pop_front();
    tokens.pop_front();
    EXPECT_EQ(0, liveCount);
  }

  EXPECT_EQ(0, liveCount);
}


} // namespace
} // namespace nioc::common
