////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <nioc/concurrent/routine.hpp>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace nioc::terminus
{
namespace
{

void publishOne(Port& port, const std::string_view topic)
{
  auto publisher = port.publisher<TestSchema>(topic);
  publisher.publish(publisher.draft());
}

// A channel takes one publisher, so many messages on a topic go through a single publisher, the way
// a real producer holds one and publishes through it repeatedly.
void publishSeveral(Port& port, const std::string_view topic, const int count)
{
  auto publisher = port.publisher<TestSchema>(topic);
  for(auto sent = 0; sent < count; ++sent)
  {
    publisher.publish(publisher.draft());
  }
}

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class ComponentTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  ComponentTest(): ScratchDirectory{unitTestDirectory()} {}

protected:
  /// @brief A Port recording into this test's own directory, with nothing wired to it.
  [[nodiscard]] Port makePort() const
  {
    return Port{
        RunContext{path(), {}, true, ""},
        [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}};
  }
};

} // namespace

TEST_F(ComponentTest, zeroCapacityThrows)
{
  auto port = makePort();
  EXPECT_THROW(
      (EarthComponent{port, 0, concurrent::BufferMode::Overwriting}),
      std::invalid_argument);
}

TEST_F(ComponentTest, emptyInboxWaits)
{
  auto port = makePort();
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST_F(ComponentTest, drainsOneMessagePerRun)
{
  auto port = makePort();
  auto component = EarthComponent{port, 4, concurrent::BufferMode::Overwriting};
  publishSeveral(port, EarthComponent::kTopic, 2);

  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST_F(ComponentTest, overwriteDropsOldestWhenFull)
{
  auto port = makePort();
  auto component = EarthComponent{port, 2, concurrent::BufferMode::Overwriting};
  constexpr auto kPublishCount = 5;
  publishSeveral(port, EarthComponent::kTopic, kPublishCount);

  // Two slots keep the newest two; the other three were overwritten.
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

TEST_F(ComponentTest, duplicateSubscriptionThrows)
{
  auto port = makePort();

  class DoubleSubscriber final: public Component
  {
  public:
    explicit DoubleSubscriber(Port& port):
      Component{"DoubleSubscriber", port, 1, concurrent::BufferMode::Unbounded}
    {
      const auto handler = [](const Message<TestSchema>&) { return State::Continue; };
      subscribe<TestSchema>("topic", handler);
      subscribe<TestSchema>("topic", handler);
    }
  };

  EXPECT_THROW(DoubleSubscriber{port}, std::logic_error);
}

TEST_F(ComponentTest, callbackFailureEndsTheComponentWithoutEscaping)
{
  auto port = makePort();
  constexpr auto kThrowingTopic = std::string_view{"throwing"};

  class ThrowingComponent final: public Component
  {
  public:
    ThrowingComponent(Port& port, const std::string_view& topic):
      Component{"ThrowingComponent", port, 1, concurrent::BufferMode::Unbounded}
    {
      subscribe<TestSchema>(
          topic,
          [](const Message<TestSchema>&) -> State
          { throw std::runtime_error{"callback failure"}; });
    }
  };

  auto component = ThrowingComponent{port, kThrowingTopic};
  publishOne(port, kThrowingTopic);

  // The exception is caught and logged; the component reports Done so its Runner winds it down.
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Done);
}

TEST_F(ComponentTest, unboundedRetainsEveryMessage)
{
  auto port = makePort();
  auto component = EarthComponent{port, 1, concurrent::BufferMode::Unbounded};
  constexpr auto kPublishCount = 5;
  publishSeveral(port, EarthComponent::kTopic, kPublishCount);

  // Unbounded keeps all five despite a nominal capacity of 1.
  for(auto count = 0; count < kPublishCount; ++count)
  {
    EXPECT_EQ(component.tick(), concurrent::Routine::State::Continue);
  }
  EXPECT_EQ(component.tick(), concurrent::Routine::State::Waiting);
}

} // namespace nioc::terminus
