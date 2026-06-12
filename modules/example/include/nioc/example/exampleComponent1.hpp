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

/// @brief Example @ref terminus::Component that turns each received `Sample3` into a `Sample2`.
///
/// A minimal request/response component showing how to subscribe and publish. It subscribes to
/// `Sample3` on one topic; for every message received it publishes one `Sample2` on another topic.
class ExampleComponent1 final: public terminus::Component
{
public:
  /// @brief Configures the component from its config block.
  ///
  /// @param port Hub the component subscribes to and publishes onto; must outlive this component.
  ///
  /// @param config View of this component's config block (see exampleComponent1Config.capnp): the
  /// input and output topics, plus a `component` subsection forwarded to the @ref
  /// terminus::Component base.
  ExampleComponent1(terminus::Port& port, ExampleComponent1Config::Reader config);

private:
  std::string mSample3Topic;
  std::string mSample2Topic;

  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);
};

} // namespace nioc::example
