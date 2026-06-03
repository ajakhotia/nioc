////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/concurrent/asyncProcessor.hpp>
#include <nioc/concurrent/routine.hpp>

#include <gtest/gtest.h>
#include <string_view>
#include <vector>

namespace nioc::concurrent
{
namespace
{

TEST(AsyncProcessor, StepWaitsWhenInboxEmpty)
{
  auto processor = AsyncProcessor<int>("test", BufferMode::Unbounded, 0, [](int) {});
  EXPECT_EQ(processor.step(), Routine::State::Waiting);
}

TEST(AsyncProcessor, ProcessesPushedValuesOnePerStepInOrder)
{
  auto received = std::vector<int>{};
  auto processor = AsyncProcessor<int>(
      "test",
      BufferMode::Unbounded,
      0,
      [&received](const int value)
      {
        received.push_back(value);
      });

  processor.push(1);
  processor.push(2);

  EXPECT_EQ(processor.step(), Routine::State::Continue);
  EXPECT_EQ(processor.step(), Routine::State::Continue);
  EXPECT_EQ(processor.step(), Routine::State::Waiting);

  EXPECT_EQ(received, (std::vector<int>{1, 2}));
}

TEST(AsyncProcessor, NameReturnsTheLabel)
{
  const auto processor = AsyncProcessor<int>("recorder", BufferMode::Unbounded, 0, [](int) {});
  EXPECT_EQ(processor.name(), std::string_view{"recorder"});
}

} // namespace
} // namespace nioc::concurrent
