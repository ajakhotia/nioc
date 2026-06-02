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

/// @brief Example @ref terminus::Component that counts the messages it receives on three topics.
///
/// A minimal sink showing how to subscribe to several topics at once. It subscribes to `Sample1`,
/// `Sample2`, and `Sample3` on their own topics; each received message bumps the matching counter
/// and logs the running totals. The per-type counts are exposed for inspection.
class ExampleComponent2 final: public terminus::Component
{
public:
  /// @brief Subscribes to all three input topics.
  ///
  /// @param port Hub the component subscribes to; must outlive this component.
  ///
  /// @param sample1Topic Topic the `Sample1` messages are read from.
  ///
  /// @param sample2Topic Topic the `Sample2` messages are read from.
  ///
  /// @param sample3Topic Topic the `Sample3` messages are read from.
  ///
  /// @param inboxCapacity Maximum number of undelivered messages held at once for a bounded
  /// @p bufferMode; ignored when unbounded.
  ///
  /// @param bufferMode Storage discipline of the inbox (see @ref concurrent::BufferMode).
  ExampleComponent2(
      terminus::Port& port,
      std::string sample1Topic,
      std::string sample2Topic,
      std::string sample3Topic,
      std::size_t inboxCapacity,
      concurrent::BufferMode bufferMode);

  /// @brief Returns the number of `Sample1` messages received so far.
  [[nodiscard]] std::uint64_t sample1Count() const noexcept;

  /// @brief Returns the number of `Sample2` messages received so far.
  [[nodiscard]] std::uint64_t sample2Count() const noexcept;

  /// @brief Returns the number of `Sample3` messages received so far.
  [[nodiscard]] std::uint64_t sample3Count() const noexcept;

private:
  void process(const terminus::ConstMsgPtr<Sample1>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample2>& msgPtr);

  void process(const terminus::ConstMsgPtr<Sample3>& msgPtr);

  void logCounts() const;

  std::string mSample1Topic;
  std::string mSample2Topic;
  std::string mSample3Topic;
  std::atomic<std::uint64_t> mSample1Count{ 0 };
  std::atomic<std::uint64_t> mSample2Count{ 0 };
  std::atomic<std::uint64_t> mSample3Count{ 0 };
};

} // namespace nioc::example
