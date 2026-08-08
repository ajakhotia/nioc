////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/common/sleep.hpp>
#include <nioc/example/config/pastureConfig.capnp.h>
#include <nioc/example/idl/wool.capnp.h>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/config.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

/// @brief A producer of wool.
///
/// Inputs:
///   - none
///
/// Outputs:
///   - Wool
///
/// Publishes one Wool (with a monotonic id) every `miningTimeMs`, then waits. The wait is read
/// from config each time, so the rate can be changed while the program runs.
class Pasture final: public terminus::Driver
{
public:
  Pasture(const std::string& name, terminus::Port& port):
    Pasture{name, port, port.materializeConfig<PastureConfig>(name)}
  {
  }

  Pasture(std::string name, terminus::Port& port, terminus::Config<PastureConfig> config):
    Driver{std::move(name), port},
    mConfig{std::move(config)},
    mWoolPublisher{publisher<Wool>(mConfig.reader().getResourceTopic().cStr())}
  {
  }

private:
  terminus::Config<PastureConfig> mConfig;
  terminus::Publisher<Wool> mWoolPublisher;
  std::uint64_t mNextWoolId{0};

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

  /// @brief Mine and publish one Wool. Stand-in for whatever real work a producer does.
  void produce()
  {
    const auto woolId = ++mNextWoolId;
    auto wool = mWoolPublisher.draft();
    auto builder = wool.builder();
    builder.setId(woolId);
    builder.setProducer(name());
    logger::info("[{}] publishing Wool #{}", name(), woolId);
    mWoolPublisher.publish(std::move(wool));
  }
};

} // namespace nioc::example
