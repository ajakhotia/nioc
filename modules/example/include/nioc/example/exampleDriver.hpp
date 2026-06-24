////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <nioc/example/config/exampleDriverConfig.capnp.h>
#include <nioc/example/idl/sample1.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief Example @ref terminus::Driver that publishes a fixed number of rounds, then finishes.
///
/// A minimal Driver to copy from. Each @ref run publishes one `Sample1` and one `Sample3` message.
/// After the configured number of rounds it reports @ref concurrent::Routine::State::Done.
class ExampleDriver final: public terminus::Driver
{
public:
  /// @brief Builds the driver from its config block.
  ///
  /// @param port Where messages are published; must outlive this driver.
  ///
  /// @param config This driver's config block (see exampleDriverConfig.capnp): the output topics,
  /// plus a `driver` subsection passed to the @ref terminus::Driver base.
  ExampleDriver(terminus::Port& port, ExampleDriverConfig::Reader config);

private:
  terminus::Publisher<Sample1> mSample1Publisher;
  terminus::Publisher<Sample3> mSample3Publisher;
  std::size_t mRound{0};

  /// @brief Publishes one round of messages, or finishes once all rounds are done.
  ///
  /// @return @ref concurrent::Routine::State::Continue while rounds remain, else @ref
  /// concurrent::Routine::State::Done.
  [[nodiscard]] State run() final;
};

} // namespace nioc::example
