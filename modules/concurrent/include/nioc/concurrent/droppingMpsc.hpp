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

/// @brief A bounded MPSC queue that drops the new value when full and keeps the queued ones.
///
/// Producers never wait. When the queue is full, a push leaves the contents unchanged and returns
/// the rejected value so the caller can count the loss. Use it when the consumer must keep the
/// values it already accepted and can afford to drop new ones. Compare @ref OverwritingMpsc, which
/// drops the oldest value instead.
///
/// Models @ref MpscQueue. See it for the producer/consumer contract.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class DroppingMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  // TODO(anurag): Define the member functions below; they are declared but not yet implemented.

  /// @brief Constructs an empty queue.
  /// @param capacity Maximum number of values held at once; must be at least 1.
  explicit DroppingMpsc(size_type capacity);

  DroppingMpsc(const DroppingMpsc&) = delete;
  DroppingMpsc(DroppingMpsc&&) noexcept = delete;
  ~DroppingMpsc() = default;
  DroppingMpsc& operator=(const DroppingMpsc&) = delete;
  DroppingMpsc& operator=(DroppingMpsc&&) noexcept = delete;

  /// @brief Enqueues @p value; if full, rejects it and returns it unchanged.
  /// @param value Value to enqueue.
  /// @return The rejected incoming value when the queue was full, otherwise nullopt.
  std::optional<value_type> push(value_type value);

  /// @brief Constructs a value in place from @p args; if full, rejects it and returns it unchanged.
  /// @param args Constructor arguments for a value_type.
  /// @return The rejected value when the queue was full, otherwise nullopt.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args);

  /// @brief Removes and returns the oldest value, or nullopt when empty. Single consumer only.
  [[nodiscard]] std::optional<value_type> tryPop();

  /// @brief Number of values currently queued. Racy; for metrics.
  [[nodiscard]] size_type size() const;

  /// @brief Fraction of capacity in use, in [0, 1]; 1.0 when full. Racy; for metrics.
  [[nodiscard]] double occupancy() const;
};

} // namespace nioc::concurrent
