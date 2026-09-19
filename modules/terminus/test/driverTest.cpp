////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/common/filesystem.hpp>
#include <nioc/terminus/consignment.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace
{

constexpr auto kTopic = std::string_view{"driverTopic"};

class CountingDriver final: public Driver
{
public:
  CountingDriver(Port& port, const int messageCount):
    Driver{"CountingDriver", port},
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
  explicit FailingDriver(Port& port): Driver{"FailingDriver", port} {}

private:
  State run() final
  {
    throw std::runtime_error{"source failure"};
  }
};

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class DriverTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  DriverTest(): ScratchDirectory{unitTestDirectory()} {}

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

TEST_F(DriverTest, publishesOntoThePortUntilDone)
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

TEST_F(DriverTest, shutdownTokenTripsWhenThePortShutsDown)
{
  auto port = makePort();
  auto driver = CountingDriver{port, 1};

  // The driver was handed the Port's shutdown token at construction; tripping the Port trips it.
  EXPECT_FALSE(driver.shutdownRequested());
  port.shutdown();
  EXPECT_TRUE(driver.shutdownRequested());
}

TEST_F(DriverTest, runFailureEndsTheDriverWithoutEscaping)
{
  auto port = makePort();
  auto driver = FailingDriver{port};

  // The exception is caught and logged; the driver reports Done so its Runner winds it down.
  EXPECT_EQ(driver.tick(), concurrent::Routine::State::Done);
}

} // namespace nioc::terminus
