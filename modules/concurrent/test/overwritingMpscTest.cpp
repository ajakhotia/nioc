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

// Both bounded queues model the contract; the reserved (unimplemented) one does so by declaration.
static_assert(MpscQueue<OverwritingMpsc<int>>);
static_assert(MpscQueue<DroppingMpsc<int>>);

TEST(OverwritingMpsc, RejectsZeroCapacity)
{
  EXPECT_ANY_THROW((OverwritingMpsc<int>{ 0 }));
}

TEST(OverwritingMpsc, PushReturnsNulloptWhileRoomRemains)
{
  auto queue = OverwritingMpsc<int>{ 3 };
  EXPECT_FALSE(queue.push(1).has_value());
  EXPECT_FALSE(queue.push(2).has_value());
  EXPECT_FALSE(queue.push(3).has_value());
  EXPECT_EQ(queue.size(), 3U);
}

TEST(OverwritingMpsc, OccupancyReflectsFillFraction)
{
  auto queue = OverwritingMpsc<int>{ 4 };
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.0);

  queue.push(1);
  queue.push(2);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 0.5);

  queue.push(3);
  queue.push(4);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 1.0);

  // At capacity, occupancy stays 1.0 as further pushes evict rather than grow.
  queue.push(5);
  EXPECT_DOUBLE_EQ(queue.occupancy(), 1.0);
}

TEST(OverwritingMpsc, PopsInFifoOrder)
{
  auto queue = OverwritingMpsc<int>{ 4 };
  queue.push(10);
  queue.push(20);
  queue.push(30);

  EXPECT_EQ(queue.tryPop(), 10);
  EXPECT_EQ(queue.tryPop(), 20);
  EXPECT_EQ(queue.tryPop(), 30);
  EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(OverwritingMpsc, EvictsOldestWhenFull)
{
  auto queue = OverwritingMpsc<int>{ 2 };
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
  auto queue = OverwritingMpsc<std::unique_ptr<int>>{ 1 };
  EXPECT_FALSE(queue.push(std::make_unique<int>(7)).has_value());

  // Full at capacity 1: the next push evicts and returns the first value.
  auto evicted = queue.push(std::make_unique<int>(8));
  ASSERT_TRUE(evicted.has_value());
  EXPECT_EQ(**evicted, 7);

  auto popped = queue.tryPop();
  ASSERT_TRUE(popped.has_value());
  EXPECT_EQ(**popped, 8);
}

// Every value that enters is later popped, evicted, or still resident — never duplicated or lost.
// pushed == popped + evicted + size() must hold exactly, whatever the interleaving.
TEST(OverwritingMpsc, ConservesValuesUnderConcurrentProducers)
{
  constexpr auto kProducers = std::size_t{ 8 };
  constexpr auto kPerProducer = std::size_t{ 10000 };
  constexpr auto kCapacity = std::size_t{ 16 };

  auto queue = OverwritingMpsc<int>{ kCapacity };
  auto evicted = std::atomic<std::size_t>{ 0 };
  auto popped = std::atomic<std::size_t>{ 0 };
  auto producersDone = std::atomic<bool>{ false };

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
  for(auto producer = std::size_t{ 0 }; producer < kProducers; ++producer)
  {
    producers.emplace_back(
        [&]
        {
          for(auto i = std::size_t{ 0 }; i < kPerProducer; ++i)
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
