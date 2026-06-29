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
#include <nioc/example/idl/grain.capnp.h>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <utility>

namespace nioc::example
{

/// @brief A producer of grain.
///
/// Inputs:
///   - none
///
/// Outputs:
///   - Grain
///
/// Publishes one Grain (with a monotonic id) every `miningTimeMs`, then waits. The wait is read
/// from config each time, so the rate can be changed while the program runs.
class Fields final: public terminus::Driver
{
public:
  Fields(terminus::Port& port, const MinerConfig::Reader config):
    Driver{port, config.getDriver()},
    mConfig{config},
    mGrainPublisher{publisher<Grain>(config.getResourceTopic().cStr())}
  {
  }

private:
  MinerConfig::Reader mConfig;
  terminus::Publisher<Grain> mGrainPublisher;
  std::uint64_t mNextGrainId{0};

  [[nodiscard]] State run() final
  {
    // A real driver blocks here on a socket, message bus, or device read. Run whatever the wait is
    // through the shutdown token so it yields promptly when the run winds down; here the "read" is
    // just a pause of miningTimeMs.
    if(common::sleepFor(shutdownToken(), std::chrono::milliseconds{mConfig.getMiningTimeMs()}))
    {
      return State::Done;
    }

    produce();
    return State::Continue;
  }

  /// @brief Mine and publish one Grain. Stand-in for whatever real work a producer does.
  void produce()
  {
    const auto grainId = ++mNextGrainId;
    auto grain = mGrainPublisher.draft();
    auto builder = grain.builder();
    builder.setId(grainId);
    builder.setProducer(name());
    logger::info("[{}] publishing Grain #{}", name(), grainId);
    mGrainPublisher.publish(std::move(grain));
  }
};

} // namespace nioc::example
