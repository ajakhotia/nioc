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
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <string>

namespace nioc::example
{

/// @brief Example @ref terminus::Component that turns each `Sample3` into a `Sample2`.
///
/// Shows how to subscribe and publish. Subscribes to `Sample3` on one topic. For each message,
/// publishes one `Sample2` on another topic.
class ExampleComponent1 final: public terminus::Component
{
public:
  /// @brief Builds the component from its config block.
  ///
  /// @param port Hub it subscribes to and publishes on. Must outlive this component.
  ///
  /// @param config This component's config block (see exampleComponent1Config.capnp): the input and
  /// output topics, plus a `component` subsection passed to the @ref terminus::Component base.
  ExampleComponent1(terminus::Port& port, ExampleComponent1Config::Reader config);

private:
  std::string mSample3Topic;
  std::string mSample2Topic;

  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);
};

} // namespace nioc::example
