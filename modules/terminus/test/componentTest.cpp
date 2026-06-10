////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <filesystem>
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
ConstMsgPtr<TestSchema> makeMessage()
{
  return std::make_shared<const Msg<TestSchema>>();
}

/// A Port recording under the default temp root. The setup builds no routines; these tests
/// construct and tick their components by hand.
Port makePort()
{
  return Port{
      std::filesystem::temp_directory_path() / "niocLogs",
      {},
      {},
      true,
      "",
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}};
}

} // namespace

TEST(ComponentTest, zeroCapacityThrows)
{
  auto port = makePort();
  EXPECT_THROW(
      (EarthComponent{port, 0, concurrent::BufferMode::Overwriting}),
      std::invalid_argument);
}

TEST(ComponentTest, emptyInboxWaits)
{
  auto port = makePort();
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, drainsOneMessagePerRun)
{
  auto port = makePort();
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  port.publish<TestSchema>(EarthComponent::kTopic, makeMessage());
  port.publish<TestSchema>(EarthComponent::kTopic, makeMessage());

  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, overwriteDropsOldestWhenFull)
{
  auto port = makePort();
  auto component = EarthComponent{port, 2, concurrent::BufferMode::Overwriting};
  constexpr auto kPublishCount = 5;
  for(auto count = 0; count < kPublishCount; ++count)
  {
    port.publish<TestSchema>(EarthComponent::kTopic, makeMessage());
  }

  // Two slots keep the newest two; the other three were overwritten.
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST(ComponentTest, unboundedRetainsEveryMessage)
{
  auto port = makePort();
  auto component = EarthComponent{port, 1, concurrent::BufferMode::Unbounded};
  constexpr auto kPublishCount = 5;
  for(auto count = 0; count < kPublishCount; ++count)
  {
    port.publish<TestSchema>(EarthComponent::kTopic, makeMessage());
  }

  // Unbounded keeps all five despite a nominal capacity of 1.
  for(auto count = 0; count < kPublishCount; ++count)
  {
    EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  }
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

} // namespace nioc::terminus
