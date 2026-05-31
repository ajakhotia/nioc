////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/routine.hpp>
#include <stdexcept>

namespace nioc::terminus
{
namespace
{

using namespace std::chrono_literals;

/// A finalized message to push. step() drains by pointer and never inspects the payload, so any
/// real message exercises the inbox identically.
ConstMsgBasePtr makeMessage()
{
  return std::make_shared<const Msg<TestSchema>>();
}

/// Arbitrary channel id; the foundation inbox tests never dispatch, so its value is irrelevant.
constexpr auto kChannel = Component::ChannelId{ 1 };

} // namespace

TEST(ComponentTest, zeroCapacityThrows)
{
  auto port = Port{};
  EXPECT_THROW((EarthComponent{ port, 0, OverflowPolicy::Overwrite }), std::invalid_argument);
}

TEST(ComponentTest, emptyInboxWaits)
{
  auto port = Port{};
  auto component = EarthComponent{ port, 4, OverflowPolicy::Overwrite };
  EXPECT_EQ(component.step(), Routine::State::Waiting);
}

TEST(ComponentTest, drainsOneMessagePerRun)
{
  auto port = Port{};
  auto component = EarthComponent{ port, 4, OverflowPolicy::Overwrite };
  component.push(kChannel, makeMessage());
  component.push(kChannel, makeMessage());

  EXPECT_EQ(component.step(), Routine::State::Continue);
  EXPECT_EQ(component.step(), Routine::State::Continue);
  EXPECT_EQ(component.step(), Routine::State::Waiting);
}

TEST(ComponentTest, overwriteDropsOldestWhenFull)
{
  auto port = Port{};
  auto component = EarthComponent{ port, 2, OverflowPolicy::Overwrite };
  for(auto count = 0; count < 5; ++count)
  {
    component.push(kChannel, makeMessage());
  }

  // Two slots keep the newest two; the other three were overwritten.
  EXPECT_EQ(component.step(), Routine::State::Continue);
  EXPECT_EQ(component.step(), Routine::State::Continue);
  EXPECT_EQ(component.step(), Routine::State::Waiting);
}

TEST(ComponentTest, blockPolicyWaitsForSpace)
{
  auto port = Port{};
  auto component = EarthComponent{ port, 1, OverflowPolicy::Block };
  component.push(kChannel, makeMessage()); // inbox is now full

  auto posted = std::atomic{ false };
  auto producer = std::thread{ [&component, &posted]
                               {
                                 component.push(kChannel, makeMessage());
                                 posted.store(true);
                               } };

  // The producer must be parked on the full inbox.
  std::this_thread::sleep_for(50ms);
  EXPECT_FALSE(posted.load());

  // Freeing a slot releases it.
  EXPECT_EQ(component.step(), Routine::State::Continue);
  producer.join();
  EXPECT_TRUE(posted.load());
}

} // namespace nioc::terminus
