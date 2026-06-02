////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/concurrent/mpscQueue.hpp>

namespace nioc::concurrent
{
namespace
{

static_assert(MpscQueue<AnyMpsc<int>>);

TEST(AnyMpsc, OverwritingModeEvictsOldestWhenFull)
{
  auto queue = AnyMpsc<int>{ BufferMode::Overwriting, 2 };
  EXPECT_FALSE(queue.push(1).has_value());
  EXPECT_FALSE(queue.push(2).has_value());

  EXPECT_EQ(queue.push(3), 1);
  EXPECT_EQ(queue.size(), 2U);
  EXPECT_EQ(queue.tryPop(), 2);
  EXPECT_EQ(queue.tryPop(), 3);
}

TEST(AnyMpsc, UnboundedModeNeverDiscards)
{
  auto queue = AnyMpsc<int>{ BufferMode::Unbounded };
  for(auto i = 0; i < 100; ++i)
  {
    EXPECT_FALSE(queue.push(i).has_value());
  }

  EXPECT_EQ(queue.size(), 100U);
  EXPECT_EQ(queue.tryPop(), 0);
}

TEST(AnyMpsc, OverwritingModeRejectsZeroCapacity)
{
  EXPECT_ANY_THROW((AnyMpsc<int>{ BufferMode::Overwriting, 0 }));
}

TEST(AnyMpsc, OccupancyForwardsToChosenStorage)
{
  auto bounded = AnyMpsc<int>{ BufferMode::Overwriting, 2 };
  bounded.push(1);
  EXPECT_DOUBLE_EQ(bounded.occupancy(), 0.5);

  auto unbounded = AnyMpsc<int>{ BufferMode::Unbounded };
  unbounded.push(1);
  EXPECT_DOUBLE_EQ(unbounded.occupancy(), 0.0);
}

} // namespace
} // namespace nioc::concurrent
