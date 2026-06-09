////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/concurrent/mpscQueue.hpp>
#include <nioc/concurrent/notifyingInbox.hpp>
#include <nioc/concurrent/unboundedMpsc.hpp>

#include <cstddef>
#include <gtest/gtest.h>

namespace nioc::concurrent
{
namespace
{

// Notification composes: an inbox over a concrete queue and over a runtime-selected one both model
// the queue contract themselves.
static_assert(MpscQueue<NotifyingInbox<UnboundedMpsc<int>>>);
static_assert(MpscQueue<NotifyingInbox<AnyMpsc<int>>>);

TEST(NotifyingInbox, FiresNotifyOnEveryPush)
{
  auto notifications = std::size_t{0};
  auto inbox = NotifyingInbox<UnboundedMpsc<int>>{[&notifications] { ++notifications; }};

  inbox.push(1);
  inbox.push(2);
  inbox.push(3);

  EXPECT_EQ(notifications, 3U);
}

TEST(NotifyingInbox, ForwardsPushAndPop)
{
  auto inbox = NotifyingInbox<UnboundedMpsc<int>>{nullptr};

  EXPECT_FALSE(inbox.push(10).has_value());
  EXPECT_EQ(inbox.size(), 1U);
  EXPECT_EQ(inbox.tryPop(), 10);
  EXPECT_FALSE(inbox.tryPop().has_value());
}

TEST(NotifyingInbox, ForwardsSacrificedValueFromBoundedQueue)
{
  auto inbox = NotifyingInbox<AnyMpsc<int>>{nullptr, BufferMode::Overwriting, 1};

  EXPECT_FALSE(inbox.push(1).has_value());
  EXPECT_EQ(inbox.push(2), 1);
}

TEST(NotifyingInbox, ForwardsOccupancy)
{
  auto inbox = NotifyingInbox<AnyMpsc<int>>{nullptr, BufferMode::Overwriting, 2};

  EXPECT_DOUBLE_EQ(inbox.occupancy(), 0.0);
  inbox.push(1);
  EXPECT_DOUBLE_EQ(inbox.occupancy(), 0.5);
}

} // namespace
} // namespace nioc::concurrent
