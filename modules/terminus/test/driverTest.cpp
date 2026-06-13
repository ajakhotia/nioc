////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/terminus/consignment.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{
namespace
{

constexpr auto kTopic = std::string_view{"driverTopic"};

/// A Port recording under the default temp root; the tests drive their routines by hand.
Port makePort()
{
  return Port{
      Manifest{
               RunContext{std::filesystem::temp_directory_path() / "niocLogs", {}, true, ""},
               ConfigStore{{}, {}}},
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}
  };
}

/// Publishes one value per run, reporting Done once the scripted count is exhausted.
class CountingDriver final: public Driver
{
public:
  CountingDriver(Port& port, const int messageCount):
    Driver{port, "CountingDriver"},
    mRemaining{messageCount}
  {
  }

  /// Exposes the protected token's state, so a test can observe the driver's view of a shutdown.
  [[nodiscard]] bool shutdownRequested() const
  {
    return shutdownToken().stop_requested();
  }

private:
  int mRemaining;

  State run() final
  {
    if(mRemaining <= 0)
    {
      return State::Done;
    }

    auto msg = std::make_shared<Msg<TestSchema>>();
    msg->builder().setValue(mRemaining);
    publish<TestSchema>(kTopic, std::move(msg));

    --mRemaining;
    return mRemaining > 0 ? State::Continue : State::Done;
  }
};

/// Fails on its first run; the base must convert the exception into a clean finish.
class FailingDriver final: public Driver
{
public:
  explicit FailingDriver(Port& port): Driver{port, "FailingDriver"} {}

private:
  State run() final
  {
    throw std::runtime_error{"source failure"};
  }
};

} // namespace

TEST(DriverTest, publishesOntoThePortUntilDone)
{
  auto port = makePort();

  auto received = std::vector<std::int64_t>{};
  port.subscribe(
      makeChannelId(Msg<TestSchema>::kMsgId, kTopic),
      [&received](const Consignment consignment)
      {
        received.push_back(std::static_pointer_cast<const Msg<TestSchema>>(consignment.msg())
                               ->reader()
                               .getValue());
      });

  auto driver = CountingDriver{port, 2};
  EXPECT_EQ(driver.tick(), concurrent::Routine::State::Continue);
  EXPECT_EQ(driver.tick(), concurrent::Routine::State::Done);

  EXPECT_EQ((std::vector<std::int64_t>{2, 1}), received);
}

TEST(DriverTest, shutdownTokenTripsWhenThePortShutsDown)
{
  auto port = makePort();
  auto driver = CountingDriver{port, 1};

  // The driver was handed the Port's shutdown token at construction; tripping the Port trips it.
  EXPECT_FALSE(driver.shutdownRequested());
  port.shutdown();
  EXPECT_TRUE(driver.shutdownRequested());
}

TEST(DriverTest, runFailureEndsTheDriverWithoutEscaping)
{
  auto port = makePort();
  auto driver = FailingDriver{port};

  // The exception is caught and logged; the driver reports Done so its Runner winds it down.
  EXPECT_EQ(driver.tick(), concurrent::Routine::State::Done);
}

} // namespace nioc::terminus
