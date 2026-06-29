////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/common/sleep.hpp>
#include <nioc/example/config/minerConfig.capnp.h>
#include <nioc/example/idl/ore.capnp.h>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <utility>

namespace nioc::example
{

/// @brief A producer of ore.
///
/// Inputs:
///   - none
///
/// Outputs:
///   - Ore
///
/// Publishes one Ore (with a monotonic id) every `miningTimeMs`, then waits. The wait is read
/// from config each time, so the rate can be changed while the program runs.
class Mountains final: public terminus::Driver
{
public:
  Mountains(terminus::Port& port, const MinerConfig::Reader config):
    Driver{port, config.getDriver()},
    mConfig{config},
    mOrePublisher{publisher<Ore>(config.getResourceTopic().cStr())}
  {
  }

private:
  MinerConfig::Reader mConfig;
  terminus::Publisher<Ore> mOrePublisher;
  std::uint64_t mNextOreId{0};

  [[nodiscard]] State run() final
  {
    // A real driver blocks here on a socket, message bus, or device read. Run whatever the wait is
    // through the shutdown token so it yields promptly when the run winds down; here the "read" is
    // just a pause of miningTimeMs.
    if(common::interruptibleSleepFor(
           shutdownToken(),
           std::chrono::milliseconds{mConfig.getMiningTimeMs()}))
    {
      return State::Done;
    }

    produce();
    return State::Continue;
  }

  /// @brief Mine and publish one Ore. Stand-in for whatever real work a producer does.
  void produce()
  {
    const auto oreId = ++mNextOreId;
    auto ore = mOrePublisher.draft();
    auto builder = ore.builder();
    builder.setId(oreId);
    builder.setProducer(name());
    logger::info("[{}] publishing Ore #{}", name(), oreId);
    mOrePublisher.publish(std::move(ore));
  }
};

} // namespace nioc::example
