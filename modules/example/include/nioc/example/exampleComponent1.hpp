////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <nioc/example/idl/sample2.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <string>
#include <string_view>

namespace nioc::example
{

/// @brief Example @ref terminus::Component that turns each received `Sample3` into a `Sample2`.
///
/// A minimal request/response component showing how to subscribe and publish. It subscribes to
/// `Sample3` on one topic; for every message received it publishes one `Sample2` on another topic.
class ExampleComponent1 final: public terminus::Component
{
public:
  /// @brief Subscribes to the input topic and records the output topic.
  ///
  /// @param port Hub the component subscribes to and publishes onto; must outlive this component.
  ///
  /// @param sample3Topic Topic the incoming `Sample3` messages are read from.
  ///
  /// @param sample2Topic Topic the outgoing `Sample2` messages are published on.
  ///
  /// @param inboxCapacity Maximum number of undelivered messages held at once; must be at least 1.
  ///
  /// @param overflowPolicy Behavior when a message arrives while the inbox is full.
  ExampleComponent1(
      terminus::Port& port,
      std::string sample3Topic,
      std::string sample2Topic,
      std::size_t inboxCapacity,
      terminus::OverflowPolicy overflowPolicy);

  /// @brief Returns the human-readable routine name, `"ExampleComponent1"`.
  [[nodiscard]] std::string_view name() const final;

private:
  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);

  std::string mSample3Topic;
  std::string mSample2Topic;
};

} // namespace nioc::example
