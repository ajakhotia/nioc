////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nioc/example/config/exampleComponent1Config.capnp.h>
#include <nioc/example/idl/sample2.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief A reference terminus Component that publishes one fixed Sample2 every time it receives a
/// Sample3.
///
/// It subscribes to a Sample3 topic and publishes to a Sample2 topic, both named in its config. The
/// inbound Sample3 payload is ignored; each delivery triggers one outbound Sample2 with fixed
/// fields. Use it as a worked example of wiring a Component's subscription and publisher together.
///
/// As a Component it is driven by the surrounding runtime: messages are queued on the base class
/// inbox and handled one at a time on the component's own tick, not on the publishing thread.
/// Construct it, then let the runtime drive it. Non-copyable and non-movable.
///
/// @see terminus::Component, terminus::Publisher
class ExampleComponent1 final: public terminus::Component
{
public:
  /// @brief Subscribe to the config's Sample3 topic and open a publisher on its Sample2 topic.
  ///
  /// @param port The owning Port that delivers and routes messages. Must outlive the component.
  ///
  /// @param config Supplies the Sample3 and Sample2 topic names and the base Component config.
  /// Read only during construction.
  ExampleComponent1(terminus::Port& port, ExampleComponent1Config::Reader config);

private:
  /// The open publisher on the config's Sample2 topic. Opened during construction and used by
  /// process to emit each outbound Sample2.
  terminus::Publisher<Sample2> mSample2Publisher;

  /// @brief Build and publish one Sample2 with fixed fields, ignoring the inbound Sample3.
  ///
  /// Runs on the component's tick, one message at a time. @p message is unused.
  void process(const terminus::Message<Sample3>& message);
};

} // namespace nioc::example
