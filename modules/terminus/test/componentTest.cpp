////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <nioc/concurrent/routine.hpp>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <stdexcept>

namespace nioc::terminus
{
namespace
{

/// A finalized message to publish. step() drains by pointer and the test handler never inspects the
/// payload, so any real message exercises the inbox identically.
ConstMsgBasePtr makeMessage()
{
  return std::make_shared<const Msg<TestSchema>>();
}

/// Channel EarthComponent subscribes to; publishing here feeds its inbox and lets step() dispatch.
auto channelId()
{
  return makeChannelId(Msg<TestSchema>::kMsgId, EarthComponent::kTopic);
}

} // namespace

TEST(ComponentTest, zeroCapacityThrows)
{
  auto port = Port{};
  EXPECT_THROW(
      (EarthComponent{port, 0, concurrent::BufferMode::Overwriting}),
      std::invalid_argument);
}

TEST(ComponentTest, emptyInboxWaits)
{
  auto port = Port{};
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, drainsOneMessagePerRun)
{
  auto port = Port{};
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  port.publish(channelId(), makeMessage());
  port.publish(channelId(), makeMessage());

  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, overwriteDropsOldestWhenFull)
{
  auto port = Port{};
  auto component = EarthComponent{port, 2, concurrent::BufferMode::Overwriting};
  constexpr auto kPublishCount = 5;
  for(auto count = 0; count < kPublishCount; ++count)
  {
    port.publish(channelId(), makeMessage());
  }

  // Two slots keep the newest two; the other three were overwritten.
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, unboundedRetainsEveryMessage)
{
  auto port = Port{};
  auto component = EarthComponent{port, 1, concurrent::BufferMode::Unbounded};
  constexpr auto kPublishCount = 5;
  for(auto count = 0; count < kPublishCount; ++count)
  {
    port.publish(channelId(), makeMessage());
  }

  // Unbounded keeps all five despite a nominal capacity of 1.
  for(auto count = 0; count < kPublishCount; ++count)
  {
    EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  }
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

} // namespace nioc::terminus
