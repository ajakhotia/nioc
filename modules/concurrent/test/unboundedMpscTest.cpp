////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <nioc/concurrent/mpscQueue.hpp>
#include <nioc/concurrent/unboundedMpsc.hpp>

#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

namespace nioc::concurrent
{
namespace
{

static_assert(MpscQueue<UnboundedMpsc<int>>);

TEST(UnboundedMpsc, PushNeverDiscards)
{
  auto queue = UnboundedMpsc<int>{};
  EXPECT_FALSE(queue.push(1).has_value());
  EXPECT_FALSE(queue.push(2).has_value());
  EXPECT_EQ(queue.size(), 2U);
}

TEST(UnboundedMpsc, OccupancyIsAlwaysZeroWhileSizeTracksBacklog)
{
  auto queue = UnboundedMpsc<int>{};
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.0);

  queue.push(1);
  queue.push(2);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.0);
  EXPECT_EQ(queue.size(), 2U);
}

TEST(UnboundedMpsc, PopsInFifoOrder)
{
  auto queue = UnboundedMpsc<int>{};
  queue.push(10);
  queue.push(20);
  queue.push(30);

  EXPECT_EQ(queue.tryPop(), 10);
  EXPECT_EQ(queue.tryPop(), 20);
  EXPECT_EQ(queue.tryPop(), 30);
  EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(UnboundedMpsc, SupportsMoveOnlyValues)
{
  auto queue = UnboundedMpsc<std::unique_ptr<int>>{};
  EXPECT_FALSE(queue.push(std::make_unique<int>(7)).has_value());

  auto popped = queue.tryPop();
  ASSERT_TRUE(popped.has_value());
  EXPECT_EQ(**popped, 7);
}

// Lossless and duplicate-free: with every producer emitting a disjoint range of values, the
// consumer must receive exactly the union [0, kTotal) regardless of interleaving.
TEST(UnboundedMpsc, DeliversEveryValueUnderConcurrentProducers)
{
  constexpr auto kProducers = std::size_t{8};
  constexpr auto kPerProducer = std::size_t{10000};
  constexpr auto kTotal = kProducers * kPerProducer;

  auto queue = UnboundedMpsc<std::size_t>{};
  auto received = std::vector<std::size_t>{};
  received.reserve(kTotal);

  auto consumer = std::thread(
      [&]
      {
        while(received.size() < kTotal)
        {
          if(auto value = queue.tryPop())
          {
            received.push_back(*value);
          }
        }
      });

  auto producers = std::vector<std::thread>{};
  for(auto producer = std::size_t{0}; producer < kProducers; ++producer)
  {
    producers.emplace_back(
        [&queue, producer]
        {
          for(auto i = std::size_t{0}; i < kPerProducer; ++i)
          {
            queue.push((producer * kPerProducer) + i);
          }
        });
  }

  for(auto& producer: producers)
  {
    producer.join();
  }
  consumer.join();

  std::ranges::sort(received);
  auto expected = std::vector<std::size_t>(kTotal);
  std::ranges::iota(expected, std::size_t{0});
  EXPECT_EQ(received, expected);
}

} // namespace
} // namespace nioc::concurrent
