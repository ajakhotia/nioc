////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/concurrent/droppingMpsc.hpp>
#include <nioc/concurrent/mpscQueue.hpp>
#include <nioc/concurrent/overwritingMpsc.hpp>

#include <atomic>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

namespace nioc::concurrent
{
namespace
{

// Both bounded queues model the MpscQueue contract.
static_assert(MpscQueue<OverwritingMpsc<int>>);
static_assert(MpscQueue<DroppingMpsc<int>>);

TEST(OverwritingMpsc, RejectsZeroCapacity)
{
  EXPECT_ANY_THROW((OverwritingMpsc<int>{0}));
}

TEST(OverwritingMpsc, PushReturnsNulloptWhileRoomRemains)
{
  auto queue = OverwritingMpsc<int>{3};
  EXPECT_FALSE(queue.push(1).has_value());
  EXPECT_FALSE(queue.push(2).has_value());
  EXPECT_FALSE(queue.push(3).has_value());
  EXPECT_EQ(queue.size(), 3U);
}

TEST(OverwritingMpsc, OccupancyReflectsFillFraction)
{
  constexpr auto overflowValue = 5;

  auto queue = OverwritingMpsc<int>{4};
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.0);

  queue.push(1);
  queue.push(2);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.5);

  queue.push(3);
  queue.push(4);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 1.0);

  // At capacity, occupancy stays 1.0 as further pushes evict rather than grow.
  queue.push(overflowValue);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 1.0);
}

TEST(OverwritingMpsc, PopsInFifoOrder)
{
  constexpr auto firstValue = 10;
  constexpr auto secondValue = 20;
  constexpr auto thirdValue = 30;

  auto queue = OverwritingMpsc<int>{4};
  queue.push(firstValue);
  queue.push(secondValue);
  queue.push(thirdValue);

  EXPECT_EQ(queue.tryPop(), firstValue);
  EXPECT_EQ(queue.tryPop(), secondValue);
  EXPECT_EQ(queue.tryPop(), thirdValue);
  EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(OverwritingMpsc, EvictsOldestWhenFull)
{
  auto queue = OverwritingMpsc<int>{2};
  EXPECT_FALSE(queue.push(1).has_value());
  EXPECT_FALSE(queue.push(2).has_value());

  // Full now: each further push evicts and returns the oldest queued value.
  EXPECT_EQ(queue.push(3), 1);
  EXPECT_EQ(queue.push(4), 2);
  EXPECT_EQ(queue.size(), 2U);

  EXPECT_EQ(queue.tryPop(), 3);
  EXPECT_EQ(queue.tryPop(), 4);
}

TEST(OverwritingMpsc, SupportsMoveOnlyValues)
{
  constexpr auto firstValue = 7;
  constexpr auto secondValue = 8;

  auto queue = OverwritingMpsc<std::unique_ptr<int>>{1};
  EXPECT_FALSE(queue.push(std::make_unique<int>(firstValue)).has_value());

  // Full at capacity 1: the next push evicts and returns the first value.
  const auto evicted = queue.push(std::make_unique<int>(secondValue));
  ASSERT_TRUE(evicted.has_value());
  if(evicted.has_value())
  {
    EXPECT_EQ(**evicted, firstValue);
  }

  const auto popped = queue.tryPop();
  ASSERT_TRUE(popped.has_value());
  if(popped.has_value())
  {
    EXPECT_EQ(**popped, secondValue);
  }
}

// Every value that enters is later popped, evicted, or still resident — never duplicated or lost.
// pushed == popped + evicted + size() must hold exactly, whatever the interleaving.
TEST(OverwritingMpsc, ConservesValuesUnderConcurrentProducers)
{
  constexpr auto kProducers = std::size_t{8};
  constexpr auto kPerProducer = std::size_t{10000};
  constexpr auto kCapacity = std::size_t{16};

  auto queue = OverwritingMpsc<int>{kCapacity};
  auto evicted = std::atomic<std::size_t>{0};
  auto popped = std::atomic<std::size_t>{0};
  auto producersDone = std::atomic<bool>{false};

  auto consumer = std::thread(
      [&]
      {
        const auto drain = [&]
        {
          while(queue.tryPop().has_value())
          {
            popped.fetch_add(1, std::memory_order_relaxed);
          }
        };

        while(not producersDone.load(std::memory_order_acquire))
        {
          drain();
        }
        drain();
      });

  auto producers = std::vector<std::thread>{};
  for(auto producer = std::size_t{0}; producer < kProducers; ++producer)
  {
    producers.emplace_back(
        [&]
        {
          for(auto i = std::size_t{0}; i < kPerProducer; ++i)
          {
            if(queue.push(1).has_value())
            {
              evicted.fetch_add(1, std::memory_order_relaxed);
            }
          }
        });
  }

  for(auto& producer: producers)
  {
    producer.join();
  }
  producersDone.store(true, std::memory_order_release);
  consumer.join();

  EXPECT_EQ(kProducers * kPerProducer, popped.load() + evicted.load() + queue.size());
}

} // namespace
} // namespace nioc::concurrent
