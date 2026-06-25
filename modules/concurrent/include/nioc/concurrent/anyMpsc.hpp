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

/// @brief Picks which overflow policy an @ref AnyMpsc uses, chosen at construction time.
///
/// @see AnyMpsc
enum class BufferMode : std::uint8_t
{
  /// Fixed-capacity ring buffer. When full, a push drops the oldest element to admit the new one.
  Overwriting,

  /// Grows without limit. A push never drops and never blocks; bounded only by memory.
  Unbounded
};

/// @brief A thread-safe multi-producer single-consumer queue whose overflow policy
/// (overwrite-oldest or grow-unbounded) is selected at runtime instead of at compile time.
///
/// Models @ref MpscQueue. Holds either an @ref OverwritingMpsc or an @ref UnboundedMpsc, chosen by
/// the @ref BufferMode passed to the constructor. Use this when the policy is a runtime decision
/// (e.g. from config) so call sites need not be templated on it; if the policy is fixed at compile
/// time, use the concrete queue type directly. Any number of producers may call @ref push /
/// @ref emplace concurrently with a single consumer calling @ref tryPop.
///
/// Example:
///
///     // 256-slot ring that drops the oldest frame when full.
///     AnyMpsc<Frame> queue{BufferMode::Overwriting, 256};
///     queue.push(frame);                 // any producer thread
///     auto next = queue.tryPop();        // the one consumer thread
///
/// Neither copyable nor movable: pin it in place (e.g. behind a pointer) and share it by reference.
///
/// @tparam ValueType The element type. Stored and returned by value.
///
/// @see BufferMode, MpscQueue, OverwritingMpsc, UnboundedMpsc
template<typename ValueType>
class AnyMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  /// @brief Construct a queue that uses the overflow policy named by @p mode.
  ///
  /// @param mode Selects the backing queue: overwrite-oldest or grow-unbounded.
  ///
  /// @param capacity Ring size for @c BufferMode::Overwriting; must be at least 1. Ignored for
  /// @c BufferMode::Unbounded.
  ///
  /// @throws std::invalid_argument if @p mode is @c Overwriting and @p capacity is 0.
  explicit AnyMpsc(const BufferMode mode, const size_type capacity = 0):
    mStorage{makeStorage(mode, capacity)}
  {
  }

  AnyMpsc(const AnyMpsc&) = delete;
  AnyMpsc(AnyMpsc&&) noexcept = delete;
  ~AnyMpsc() = default;
  AnyMpsc& operator=(const AnyMpsc&) = delete;
  AnyMpsc& operator=(AnyMpsc&&) noexcept = delete;

  /// @brief Enqueue @p value, returning the element evicted to make room, or @c nullopt if none
  /// was evicted.
  ///
  /// In @c Overwriting mode a full queue returns its dropped oldest element; in @c Unbounded mode
  /// the result is always @c nullopt. Safe to call from any producer thread.
  std::optional<value_type> push(value_type value)
  {
    return std::visit([&value](auto& queue) { return queue.push(std::move(value)); }, mStorage);
  }

  /// @brief Enqueue an element constructed in place from @p args; otherwise behaves like @ref push.
  ///
  /// @tparam Args Constructor argument types for @c value_type, constrained so that @c value_type
  /// must be constructible from them.
  ///
  /// @param args Arguments forwarded to the @c value_type constructor.
  ///
  /// @see push
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    return std::visit(
        [&](auto& queue) { return queue.emplace(std::forward<Args>(args)...); },
        mStorage);
  }

  /// @brief Remove and return the oldest element, or @c nullopt if the queue is empty.
  ///
  /// Call from the single consumer thread only; concurrent consumers are not supported.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return std::visit([](auto& queue) { return queue.tryPop(); }, mStorage);
  }

  /// @brief Current number of queued elements.
  ///
  /// A momentary snapshot that may be stale by the time it returns under concurrent access; use for
  /// metrics, not for control flow.
  [[nodiscard]] size_type size() const
  {
    return std::visit([](const auto& queue) { return queue.size(); }, mStorage);
  }

  /// @brief The fraction of capacity currently filled, in [0, 1] (size / capacity) for
  /// @c Overwriting; always 0.0 for @c Unbounded, which has no capacity.
  ///
  /// A momentary snapshot, like @ref size.
  ///
  /// @see size
  [[nodiscard]] double occupancy() const
  {
    return std::visit([](const auto& queue) { return queue.occupancy(); }, mStorage);
  }

private:
  using Storage = std::variant<OverwritingMpsc<value_type>, UnboundedMpsc<value_type>>;

  Storage mStorage;

  /// @brief Build the @ref Storage alternative selected by @p mode, ready to initialize
  /// @c mStorage.
  ///
  /// Returns the variant by prvalue so the member is built in place without a move, which the
  /// mutex-holding queues do not provide. Called only by the constructor.
  ///
  /// @param mode Selects which alternative to construct.
  ///
  /// @param capacity Forwarded as the ring size to @ref OverwritingMpsc; unused for
  /// @ref UnboundedMpsc.
  ///
  /// @throws std::invalid_argument if @p mode is @c Overwriting and @p capacity is 0.
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
