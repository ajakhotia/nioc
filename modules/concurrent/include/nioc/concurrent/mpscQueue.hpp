////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <concepts>
#include <optional>
#include <utility>

namespace nioc::concurrent
{

/// @brief A multi-producer, single-consumer queue of values.
///
/// Many threads may enqueue at the same time with @ref push or @ref emplace. Exactly one thread
/// may dequeue with @ref tryPop. No operation blocks: a bounded queue at capacity drops a value
/// instead of waiting; an unbounded queue grows.
///
/// @ref push and @ref emplace return the value they dropped, if any. A bounded queue that is full
/// returns the dropped value (the evicted oldest or the rejected new one, depending on its policy);
/// a queue with room returns nullopt. What a drop means is up to the caller.
///
/// Enqueue and dequeue move values in and out; they never hand back a reference into the queue.
/// That is what keeps them safe across concurrent producers and a value-dropping enqueue.
///
/// The queue handles its own locking, so no external lock is needed. The type cannot enforce the
/// single-consumer rule: only one thread may ever call @ref tryPop.
template<typename Queue>
concept MpscQueue =
    requires(Queue queue, const Queue& constQueue, typename Queue::value_type value) {
      typename Queue::value_type;
      typename Queue::size_type;

      // Enqueue by move. Returns the dropped value, or nullopt if none was dropped.
      { queue.push(std::move(value)) } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Construct a value in place from forwarded arguments. Same return as push.
      {
        queue.emplace(std::move(value))
      } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Remove and return the oldest value, or nullopt when empty. Single consumer only.
      { queue.tryPop() } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Number of values queued. Racy under concurrent producers; for metrics only. The only
      // backlog signal an unbounded queue offers.
      { constQueue.size() } -> std::same_as<typename Queue::size_type>;

      // Fraction of capacity in use, in [0, 1]; 1.0 when full. An unbounded queue always
      // reports 0.0. Racy; shows how close a bounded queue is to dropping.
      { constQueue.occupancy() } -> std::same_as<double>;
    };

} // namespace nioc::concurrent
