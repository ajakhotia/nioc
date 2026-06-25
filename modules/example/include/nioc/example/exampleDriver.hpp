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

/// @brief A worked example of a Terminus source driver: each tick it publishes one Sample1 and one
/// Sample3 message until shutdown is requested.
///
/// Use it as a template for writing your own driver. The pattern is: open publishers from config in
/// the constructor, then publish on each topic inside run(). A per-tick counter fills Sample1's
/// `value` and toggles Sample3's `flag` so the output changes over time.
///
/// Register the driver with the Port during wiring; the Port then owns it via `shared_ptr` and a
/// Runner ticks it. You do not call run() yourself.
///
/// @see terminus::Driver, terminus::Port
class ExampleDriver final: public terminus::Driver
{
public:
  /// @brief Names the driver, opens the two publishers, and registers their topics for recording.
  ///
  /// Topic names come from @p config; the publishers stay open for the driver's whole lifetime.
  ///
  /// @param port The Port this driver publishes onto. Must outlive the driver.
  ///
  /// @param config Supplies the driver name plus the Sample1 and Sample3 topic names.
  ///
  /// @throws std::invalid_argument When the config name is empty.
  ///
  /// @throws std::logic_error When the Port is not recording a chronicle, so its topics cannot be
  /// registered.
  ExampleDriver(terminus::Port& port, ExampleDriverConfig::Reader config);

private:
  terminus::Publisher<Sample1> mSample1Publisher;
  terminus::Publisher<Sample3> mSample3Publisher;

  /// The tick counter that supplies Sample1's value, and whose parity supplies Sample3's flag.
  std::size_t mRound{0};

  /// @brief Publishes one Sample1 and one Sample3, then advances the tick counter.
  ///
  /// @return State::Done once the shutdown token is signalled, State::Continue otherwise.
  [[nodiscard]] State run() final;
};

} // namespace nioc::example
