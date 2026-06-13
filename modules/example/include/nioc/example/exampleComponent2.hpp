////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <nioc/example/config/exampleComponent2Config.capnp.h>
#include <nioc/example/idl/sample1.capnp.h>
#include <nioc/example/idl/sample2.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <string>

namespace nioc::example
{

/// @brief Example @ref terminus::Component that counts messages on three topics.
///
/// Subscribes to `Sample1`, `Sample2`, and `Sample3`, each on its own topic. Each message bumps
/// the matching counter and logs the totals. Counts are readable per type.
class ExampleComponent2 final: public terminus::Component
{
public:
  /// @brief Builds the component from its config block.
  ///
  /// @param port Hub to subscribe to. Must outlive this component.
  ///
  /// @param config This component's config block (see exampleComponent2Config.capnp): the three
  /// input topics and a `component` subsection for the @ref terminus::Component base.
  ExampleComponent2(terminus::Port& port, ExampleComponent2Config::Reader config);

  /// @brief Returns how many `Sample1` messages were received.
  [[nodiscard]] std::uint64_t sample1Count() const noexcept;

  /// @brief Returns how many `Sample2` messages were received.
  [[nodiscard]] std::uint64_t sample2Count() const noexcept;

  /// @brief Returns how many `Sample3` messages were received.
  [[nodiscard]] std::uint64_t sample3Count() const noexcept;

private:
  std::string mSample1Topic;
  std::string mSample2Topic;
  std::string mSample3Topic;
  std::atomic<std::uint64_t> mSample1Count{0};
  std::atomic<std::uint64_t> mSample2Count{0};
  std::atomic<std::uint64_t> mSample3Count{0};

  void process(const terminus::ConstMsgPtr<Sample1>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample2>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);

  void logCounts() const;
};

} // namespace nioc::example
