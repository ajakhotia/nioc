////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/common/sleep.hpp>
#include <nioc/example/config/forestConfig.capnp.h>
#include <nioc/example/idl/lumber.capnp.h>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/config.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

/// @brief A producer of lumber.
///
/// Inputs:
///   - none
///
/// Outputs:
///   - Lumber
///
/// Publishes one Lumber (with a monotonic id) every `miningTimeMs`, then waits. The wait is read
/// from config each time, so the rate can be changed while the program runs.
class Forest final: public terminus::Driver
{
public:
  Forest(const std::string& name, terminus::Port& port):
    Forest{name, port, port.materializeConfig<ForestConfig>(name)}
  {
  }

  Forest(std::string name, terminus::Port& port, terminus::Config<ForestConfig> config):
    Driver{std::move(name), port},
    mConfig{std::move(config)},
    mLumberPublisher{publisher<Lumber>(mConfig.reader().getResourceTopic().cStr())}
  {
  }

private:
  terminus::Config<ForestConfig> mConfig;
  terminus::Publisher<Lumber> mLumberPublisher;
  std::uint64_t mNextLumberId{0};

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

  /// @brief Mine and publish one Lumber. Stand-in for whatever real work a producer does.
  void produce()
  {
    const auto lumberId = ++mNextLumberId;
    auto lumber = mLumberPublisher.draft();
    auto builder = lumber.builder();
    builder.setId(lumberId);
    builder.setProducer(name());
    logger::info("[{}] publishing Lumber #{}", name(), lumberId);
    mLumberPublisher.publish(std::move(lumber));
  }
};

} // namespace nioc::example
