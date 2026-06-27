////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/schema.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/consignment.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace
{

constexpr auto kTopic = std::string_view{"driverTopic"};

Port makePort()
{
  return Port{
      Manifest{
          RunContext{std::filesystem::temp_directory_path() / "niocLogs", {}, true, ""},
          ConfigStore{"{}", capnp::Schema::from<TestConfig>()}},
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}};
}

class CountingDriver final: public Driver
{
public:
  CountingDriver(Port& port, const int messageCount):
    Driver{port, "CountingDriver"},
    mPublisher{publisher<TestSchema>(kTopic)},
    mRemaining{messageCount}
  {
  }

  [[nodiscard]] bool shutdownRequested() const
  {
    return shutdownToken().stop_requested();
  }

private:
  Publisher<TestSchema> mPublisher;
  int mRemaining;

  State run() final
  {
    if(mRemaining <= 0)
    {
      return State::Done;
    }

    auto draft = mPublisher.draft();
    draft.builder().setValue(mRemaining);
    mPublisher.publish(std::move(draft));

    --mRemaining;
    return mRemaining > 0 ? State::Continue : State::Done;
  }
};

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
      chronicle::makeChannelId(kSchemaId<TestSchema>, kTopic),
      [&received](Consignment consignment)
      {
        const auto message = Message<TestSchema>{consignment.crate()};
        received.push_back(message.reader().getValue());
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
