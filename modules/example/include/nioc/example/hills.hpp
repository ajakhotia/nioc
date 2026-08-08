////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/common/sleep.hpp>
#include <nioc/example/config/hillsConfig.capnp.h>
#include <nioc/example/idl/brick.capnp.h>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/config.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

/// @brief A producer of brick.
///
/// Inputs:
///   - none
///
/// Outputs:
///   - Brick
///
/// Publishes one Brick (with a monotonic id) every `miningTimeMs`, then waits. The wait is read
/// from config each time, so the rate can be changed while the program runs.
class Hills final: public terminus::Driver
{
public:
  Hills(const std::string& name, terminus::Port& port):
    Hills{name, port, port.materializeConfig<HillsConfig>(name)}
  {
  }

  Hills(std::string name, terminus::Port& port, terminus::Config<HillsConfig> config):
    Driver{std::move(name), port},
    mConfig{std::move(config)},
    mBrickPublisher{publisher<Brick>(mConfig.reader().getResourceTopic().cStr())}
  {
  }

private:
  terminus::Config<HillsConfig> mConfig;
  terminus::Publisher<Brick> mBrickPublisher;
  std::uint64_t mNextBrickId{0};

  [[nodiscard]] State run() final
  {
    // A real driver blocks here on a socket, message bus, or device read. Run whatever the wait is
    // through the shutdown token so it yields promptly when the run winds down; here the "read" is
    // just a pause of miningTimeMs.
    if(common::sleepFor(
           shutdownToken(),
           std::chrono::milliseconds{mConfig.reader().getMiningTimeMs()}))
    {
      return State::Done;
    }

    produce();
    return State::Continue;
  }

  /// @brief Mine and publish one Brick. Stand-in for whatever real work a producer does.
  void produce()
  {
    const auto brickId = ++mNextBrickId;
    auto brick = mBrickPublisher.draft();
    auto builder = brick.builder();
    builder.setId(brickId);
    builder.setProducer(name());
    logger::info("[{}] publishing Brick #{}", name(), brickId);
    mBrickPublisher.publish(std::move(brick));
  }
};

} // namespace nioc::example
