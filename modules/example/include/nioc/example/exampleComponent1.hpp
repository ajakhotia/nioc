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

class ExampleComponent1 final: public terminus::Component
{
public:
  ExampleComponent1(
      terminus::Port& port,
      std::string sample3Topic,
      std::string sample2Topic,
      std::size_t inboxCapacity,
      terminus::OverflowPolicy overflowPolicy);

  [[nodiscard]] std::string_view name() const final;

private:
  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);

  std::string mSample3Topic;
  std::string mSample2Topic;
};

} // namespace nioc::example
