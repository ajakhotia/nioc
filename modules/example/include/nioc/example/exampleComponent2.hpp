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
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <string>

namespace nioc::example
{

/// @brief An example terminus component that subscribes to three sample topics and counts how many
/// messages of each type it has received.
///
/// Construct one as part of a terminus run, then read the per-type tallies through the count
/// accessors. The component subscribes to one topic each for Sample1, Sample2, and Sample3 (topic
/// names come from its config) and bumps the matching counter on every delivery. Deliveries run one
/// at a time on the component's own tick, so handling is serial regardless of the publishing
/// thread. Non-copyable and non-movable.
///
/// @see terminus::Component
class ExampleComponent2 final: public terminus::Component
{
public:
  /// @brief Subscribes to the Sample1, Sample2, and Sample3 topics named in @p config.
  ///
  /// @param port The bus port to subscribe through. Must outlive this component.
  ///
  /// @param config Supplies the component name plus the three topic names.
  ///
  /// @throws std::invalid_argument If the configured component name is empty, or its buffer mode
  /// is not a recognized value.
  ExampleComponent2(terminus::Port& port, ExampleComponent2Config::Reader config);

  /// @brief Returns the number of Sample1 messages received so far. Safe to call from any thread.
  [[nodiscard]] std::uint64_t sample1Count() const noexcept;

  /// @brief Returns the number of Sample2 messages received so far. Safe to call from any thread.
  [[nodiscard]] std::uint64_t sample2Count() const noexcept;

  /// @brief Returns the number of Sample3 messages received so far. Safe to call from any thread.
  [[nodiscard]] std::uint64_t sample3Count() const noexcept;

private:
  /// The topic name this component subscribes to for Sample1 messages. Taken from the config at
  /// construction.
  std::string mSample1Topic;

  /// The topic name this component subscribes to for Sample2 messages. Taken from the config at
  /// construction.
  std::string mSample2Topic;

  /// The topic name this component subscribes to for Sample3 messages. Taken from the config at
  /// construction.
  std::string mSample3Topic;

  /// Running tally of Sample1 messages received. Atomic so the count accessors can read it from any
  /// thread.
  std::atomic<std::uint64_t> mSample1Count{0};

  /// Running tally of Sample2 messages received. Atomic so the count accessors can read it from any
  /// thread.
  std::atomic<std::uint64_t> mSample2Count{0};

  /// Running tally of Sample3 messages received. Atomic so the count accessors can read it from any
  /// thread.
  std::atomic<std::uint64_t> mSample3Count{0};

  /// @brief Handles one delivered Sample1 message by incrementing the Sample1 tally.
  ///
  /// @param message The delivered message. Invoked by the subscription on the component's tick.
  void process(const terminus::Message<Sample1>& message);

  /// @brief Handles one delivered Sample2 message by incrementing the Sample2 tally.
  ///
  /// @param message The delivered message. Invoked by the subscription on the component's tick.
  void process(const terminus::Message<Sample2>& message);

  /// @brief Handles one delivered Sample3 message by incrementing the Sample3 tally.
  ///
  /// @param message The delivered message. Invoked by the subscription on the component's tick.
  void process(const terminus::Message<Sample3>& message);

  /// @brief Writes the current per-type tallies to the log.
  void logCounts() const;
};

} // namespace nioc::example
