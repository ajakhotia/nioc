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

/// @brief Storage mode an @ref AnyMpsc uses, chosen at runtime.
enum class BufferMode : std::uint8_t
{
  /// @brief Bounded. When full, drops the oldest value (latest wins). See @ref OverwritingMpsc.
  Overwriting,

  /// @brief Unbounded. Grows as needed and never drops a value. See @ref UnboundedMpsc.
  Unbounded
};

/// @brief An MPSC queue whose storage mode is picked at runtime by a @ref BufferMode.
///
/// Use this to pick bounded or unbounded from config instead of templating your own type on the
/// queue. Models @ref MpscQueue, so it works anywhere a concrete queue does, including as the
/// storage behind a @ref NotifyingInbox.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class AnyMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  /// @brief Builds the queue in the chosen storage mode.
  /// @param mode Storage mode to use.
  /// @param capacity Capacity for a bounded mode; ignored when @p mode is @ref
  /// BufferMode::Unbounded.
  explicit AnyMpsc(const BufferMode mode, const size_type capacity = 0):
    mStorage{makeStorage(mode, capacity)}
  {
  }

  AnyMpsc(const AnyMpsc&) = delete;
  AnyMpsc(AnyMpsc&&) noexcept = delete;
  ~AnyMpsc() = default;
  AnyMpsc& operator=(const AnyMpsc&) = delete;
  AnyMpsc& operator=(AnyMpsc&&) noexcept = delete;

  /// @brief Adds @p value to the queue. Returns the value dropped to make room, or nullopt.
  std::optional<value_type> push(value_type value)
  {
    return std::visit([&value](auto& queue) { return queue.push(std::move(value)); }, mStorage);
  }

  /// @brief Builds a value in place from @p args. Returns the value dropped to make room, or
  /// nullopt.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    return std::visit(
        [&](auto& queue) { return queue.emplace(std::forward<Args>(args)...); },
        mStorage);
  }

  /// @brief Removes and returns the oldest value, or nullopt when empty. Single consumer only.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return std::visit([](auto& queue) { return queue.tryPop(); }, mStorage);
  }

  /// @brief Number of values currently queued. Racy; for metrics.
  [[nodiscard]] size_type size() const
  {
    return std::visit([](const auto& queue) { return queue.size(); }, mStorage);
  }

  /// @brief Fraction of capacity in use, in [0, 1]; 0.0 when the chosen storage is unbounded.
  [[nodiscard]] double occupancy() const
  {
    return std::visit([](const auto& queue) { return queue.occupancy(); }, mStorage);
  }

private:
  using Storage = std::variant<OverwritingMpsc<value_type>, UnboundedMpsc<value_type>>;

  Storage mStorage;

  // Builds the variant alternative selected by mode in place and returns it by prvalue, so the
  // member's initialization elides any move — which the mutex-holding queues lack.
  static Storage makeStorage(const BufferMode mode, const size_type capacity)
  {
    switch(mode)
    {
      case BufferMode::Overwriting:
        return Storage{std::in_place_type<OverwritingMpsc<value_type>>, capacity};

      case BufferMode::Unbounded:
        return Storage{std::in_place_type<UnboundedMpsc<value_type>>};
    }

    std::unreachable();
  }
};

} // namespace nioc::concurrent
