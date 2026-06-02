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

/// @brief A bounded MPSC queue that, when full, rejects the incoming value and keeps the queued
/// ones.
///
/// Oldest-wins, the mirror of @ref OverwritingMpsc: a producer never waits, but once the queue is
/// full a push leaves the contents untouched and @ref push hands the rejected incoming value back
/// so the caller can account for the loss. Suits a consumer that must not lose history it has
/// already accepted and would rather shed new arrivals under overload.
///
/// Models @ref MpscQueue. See it for the producer/consumer contract.
///
/// @note The name is reserved and the contract pinned, but the implementation is deferred until a
/// reject-newest use case appears. The member functions are declared, not defined; instantiating a
/// call is a link-time error until then.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class DroppingMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

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
