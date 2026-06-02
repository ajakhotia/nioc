////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "overwritingMpsc.hpp"
#include "unboundedMpsc.hpp"
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

namespace nioc::concurrent
{

/// @brief The storage discipline an @ref AnyMpsc uses, chosen at runtime.
enum class BufferMode : std::uint8_t
{
  /// @brief Bounded; evict the oldest value when full (latest-wins). See @ref OverwritingMpsc.
  Overwriting,

  /// @brief Grow without bound; never discard (lossless). See @ref UnboundedMpsc.
  Unbounded
};

/// @brief An MPSC queue whose storage discipline is selected at runtime by a @ref BufferMode.
///
/// Lets a caller defer the bounded-versus-unbounded choice to configuration without templating its
/// own type on the storage. Dispatches each operation to the chosen concrete queue; the indirection
/// is a single visit over a two-alternative variant.
///
/// Models @ref MpscQueue, so it is interchangeable with a concrete queue anywhere one is accepted —
/// including as the storage behind a @ref NotifyingInbox.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class AnyMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  /// @brief Constructs the queue with the chosen storage discipline.
  /// @param mode Storage discipline to use.
  /// @param capacity Capacity of a bounded mode; ignored when the @p mode is @ref
  /// BufferMode::Unbounded.
  explicit AnyMpsc(const BufferMode mode, const size_type capacity = 0):
    mStorage{ makeStorage(mode, capacity) }
  {
  }

  AnyMpsc(const AnyMpsc&) = delete;
  AnyMpsc(AnyMpsc&&) noexcept = delete;
  ~AnyMpsc() = default;
  AnyMpsc& operator=(const AnyMpsc&) = delete;
  AnyMpsc& operator=(AnyMpsc&&) noexcept = delete;

  /// @brief Enqueues @p value through the chosen storage; returns the sacrificed value, if any.
  std::optional<value_type> push(value_type value)
  {
    return std::visit(
        [&value](auto& queue)
        {
          return queue.push(std::move(value));
        },
        mStorage);
  }

  /// @brief Constructs a value in place from @p args through the chosen storage; returns the
  /// sacrificed value, if any.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    return std::visit(
        [&](auto& queue)
        {
          return queue.emplace(std::forward<Args>(args)...);
        },
        mStorage);
  }

  /// @brief Removes and returns the oldest value, or nullopt when empty. Single consumer only.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return std::visit(
        [](auto& queue)
        {
          return queue.tryPop();
        },
        mStorage);
  }

  /// @brief Number of values currently queued. Racy; for metrics.
  [[nodiscard]] size_type size() const
  {
    return std::visit(
        [](const auto& queue)
        {
          return queue.size();
        },
        mStorage);
  }

  /// @brief Fraction of capacity in use, in [0, 1]; 0.0 when the chosen storage is unbounded.
  [[nodiscard]] double occupancy() const
  {
    return std::visit(
        [](const auto& queue)
        {
          return queue.occupancy();
        },
        mStorage);
  }

private:
  using Storage = std::variant<OverwritingMpsc<value_type>, UnboundedMpsc<value_type>>;

  // Builds the variant alternative selected by mode in place and returns it by prvalue, so the
  // member's initialization elides any move — which the mutex-holding queues lack.
  static Storage makeStorage(const BufferMode mode, const size_type capacity)
  {
    switch(mode)
    {
      case BufferMode::Overwriting:
        return Storage{ std::in_place_type<OverwritingMpsc<value_type>>, capacity };

      case BufferMode::Unbounded:
        return Storage{ std::in_place_type<UnboundedMpsc<value_type>> };
    }

    std::unreachable();
  }

  Storage mStorage;
};

} // namespace nioc::concurrent
