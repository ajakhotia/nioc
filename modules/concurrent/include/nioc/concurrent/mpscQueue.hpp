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

/// @brief Constrains a type to the multi-producer, single-consumer (MPSC) queue contract: a
/// thread-safe FIFO that many threads may enqueue into while exactly one thread dequeues.
///
/// Use this concept to write code that works with any MPSC queue.
///
/// A conforming type provides:
///   - `value_type`: the element type.
///   - `size_type`: the count type.
///   - `push(value_type)`: enqueue by move; returns the dropped value (if any) or `nullopt`.
///   - `emplace(...)`: enqueue an in-place-constructed value; same return as `push`.
///   - `tryPop()`: dequeue the oldest value, or `nullopt` if empty. Consumer thread only.
///   - `size() const`: the current element count.
///   - `occupancy() const`: the fraction of capacity in use, in [0, 1].
///
/// @tparam Queue The candidate queue type.
///
/// @see OverwritingMpsc, DroppingMpsc, UnboundedMpsc, AnyMpsc
template<typename Queue>
concept MpscQueue =
    requires(Queue queue, const Queue& constQueue, typename Queue::value_type value) {
      typename Queue::value_type;
      typename Queue::size_type;

      /// Enqueue by move. Returns the value dropped to make room when a bounded queue is full, else
      /// `nullopt`. Callable from any producer thread.
      { queue.push(std::move(value)) } -> std::same_as<std::optional<typename Queue::value_type>>;

      /// Construct an element in place and enqueue it. Same drop-return semantics as `push`.
      {
        queue.emplace(std::move(value))
      } -> std::same_as<std::optional<typename Queue::value_type>>;

      /// Remove and return the oldest value, or `nullopt` when empty. Call only from the single
      /// consumer thread.
      { queue.tryPop() } -> std::same_as<std::optional<typename Queue::value_type>>;

      /// Number of queued elements. A momentary snapshot; racy under concurrent producers, so use
      /// it for metrics, not control flow.
      { constQueue.size() } -> std::same_as<typename Queue::size_type>;

      /// Fraction of capacity in use, in [0, 1]: 1.0 when a bounded queue is full, 0.0 for an
      /// unbounded queue. A momentary snapshot; racy. Signals how close a bounded queue is to
      /// dropping.
      { constQueue.occupancy() } -> std::same_as<double>;
    };

} // namespace nioc::concurrent
