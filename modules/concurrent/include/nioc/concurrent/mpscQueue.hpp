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
/// Any number of threads enqueue concurrently with @ref push (handing over a value) or @ref emplace
/// (constructing one in place from forwarded arguments); exactly one thread drains with @ref
/// tryPop. No operation blocks — a bounded model at capacity sacrifices one value to stay within
/// capacity, an unbounded model grows — so a producer is never made to wait on the consumer. The
/// enqueue and drain operations move values rather than handing out references, so they stay safe
/// across concurrent producers and a value-sacrificing enqueue.
///
/// Both @ref push and @ref emplace return the value they had to give up, if any: a bounded model
/// that was full returns the
/// value it sacrificed (the evicted oldest, or the rejected incoming, per its policy); a model with
/// room returns nullopt. The caller decides what a loss means — count it, log it, reroute it — so
/// the queue stays free of policy beyond its own storage discipline.
///
/// A model owns its own synchronization; callers need no external lock. The single-consumer rule is
/// the one invariant the type cannot enforce: only one thread may ever call @ref tryPop.
template<typename Queue>
concept MpscQueue =
    requires(Queue queue, const Queue& constQueue, typename Queue::value_type value) {
      typename Queue::value_type;
      typename Queue::size_type;

      // Enqueue by move; return the value sacrificed to stay within capacity, or nullopt if none.
      { queue.push(std::move(value)) } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Construct a value in place from forwarded arguments; same sacrificed-value return as push.
      {
        queue.emplace(std::move(value))
      } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Remove and return the oldest value, or nullopt when empty. Single consumer only.
      { queue.tryPop() } -> std::same_as<std::optional<typename Queue::value_type>>;

      // Number of values currently queued. Racy under concurrent producers; for metrics. The only
      // backlog signal an unbounded queue offers.
      { constQueue.size() } -> std::same_as<typename Queue::size_type>;

      // Fraction of capacity in use, in [0, 1]; 1.0 when full. An unbounded queue cannot overflow
      // and always reports 0.0. Racy; the signal for how close a bounded queue is to dropping.
      { constQueue.occupancy() } -> std::same_as<double>;
    };

} // namespace nioc::concurrent
