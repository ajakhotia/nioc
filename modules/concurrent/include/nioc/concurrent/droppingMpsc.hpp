////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <concepts>
#include <cstddef>
#include <optional>

namespace nioc::concurrent
{

/// @brief A bounded multi-producer single-consumer FIFO queue that drops the incoming value when
/// full, keeping the values already queued.
///
/// Models @ref MpscQueue. Many threads may enqueue at once; exactly one thread may consume. A
/// `push` or `emplace` into a full queue leaves the queued values untouched and hands the rejected
/// value back to the caller. Choose this over @ref OverwritingMpsc when the values already in line
/// matter more than the newest arrival, so backlog should win over fresh data.
///
/// Example:
///
///     auto queue = DroppingMpsc<int>(2);  // capacity 2
///     queue.push(1);                      // nullopt; queued
///     queue.push(2);                      // nullopt; queued, now full
///     auto rejected = queue.push(3);      // *rejected == 3; 3 was dropped
///     auto first = queue.tryPop();        // *first == 1
///
/// @tparam ValueType The element type.
///
/// @see MpscQueue, OverwritingMpsc, UnboundedMpsc
template<typename ValueType>
class DroppingMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  // TODO(anurag): The member functions below are declared but not yet defined. The behavior
  // documented for each describes the intended contract, not working code; calling them will fail
  // to link until they are implemented.

  /// @brief Construct an empty queue that holds at most @p capacity values.
  ///
  /// @param capacity The maximum number of queued values. Must be at least 1.
  explicit DroppingMpsc(size_type capacity);

  DroppingMpsc(const DroppingMpsc&) = delete;
  DroppingMpsc(DroppingMpsc&&) noexcept = delete;
  ~DroppingMpsc() = default;
  DroppingMpsc& operator=(const DroppingMpsc&) = delete;
  DroppingMpsc& operator=(DroppingMpsc&&) noexcept = delete;

  /// @brief Enqueue @p value by move; safe to call from any producer thread.
  ///
  /// @param value Moved into the queue when there is room.
  ///
  /// @return @p value back when the queue was full and it was dropped; @c nullopt when it was
  /// queued.
  std::optional<value_type> push(value_type value);

  /// @brief Enqueue a value constructed in place from @p args; safe to call from any producer
  /// thread.
  ///
  /// @tparam Args Constructor argument types for @c value_type.
  ///
  /// @param args Forwarded to the @c value_type constructor to build the value in place.
  ///
  /// @return The constructed value when the queue was full and it was dropped; @c nullopt when it
  /// was queued.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args);

  /// @brief Remove and return the oldest queued value.
  ///
  /// Call from the single consumer thread only.
  ///
  /// @return The oldest value, or @c nullopt when the queue is empty.
  [[nodiscard]] std::optional<value_type> tryPop();

  /// @brief Report the current number of queued values.
  ///
  /// A momentary snapshot; racy under concurrent producers and may be stale once it returns. Use
  /// for metrics, not control flow.
  [[nodiscard]] size_type size() const;

  /// @brief Report the fraction filled, in [0, 1], where 1.0 means the next push will drop.
  ///
  /// A momentary snapshot; racy. Use for metrics, not control flow.
  [[nodiscard]] double occupancy() const;
};

} // namespace nioc::concurrent
