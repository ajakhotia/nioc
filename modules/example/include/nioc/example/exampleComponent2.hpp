////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <cstddef>
#include <nioc/example/idl/sample1.capnp.h>
#include <nioc/example/idl/sample2.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <string>
#include <string_view>

namespace nioc::example
{

class ExampleComponent2 final: public terminus::Component
{
public:
  ExampleComponent2(
      terminus::Port& port,
      std::string sample1Topic,
      std::string sample2Topic,
      std::string sample3Topic,
      std::size_t inboxCapacity,
      terminus::OverflowPolicy overflowPolicy);

  [[nodiscard]] std::string_view name() const final;

  [[nodiscard]] std::uint64_t sample1Count() const noexcept;

  [[nodiscard]] std::uint64_t sample2Count() const noexcept;

  [[nodiscard]] std::uint64_t sample3Count() const noexcept;

private:
  void process(const terminus::ConstMsgPtr<Sample1>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample2>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);

  std::string mSample1Topic;
  std::string mSample2Topic;
  std::string mSample3Topic;
  std::atomic<std::uint64_t> mSample1Count{ 0 };
  std::atomic<std::uint64_t> mSample2Count{ 0 };
  std::atomic<std::uint64_t> mSample3Count{ 0 };
};

} // namespace nioc::example
