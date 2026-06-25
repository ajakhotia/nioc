////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <concepts>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace nioc::concurrent
{

/// @brief A thread-safe FIFO queue that many threads may enqueue into while one thread dequeues,
/// and that has no capacity limit so it never drops a value.
///
/// Models @ref MpscQueue. Any number of producer threads may call push and emplace at the same
/// time; exactly one consumer thread may call tryPop. Because it has no capacity, push and emplace
/// always accept the value and return nullopt -- nothing is ever evicted. The queue grows until it
/// exhausts available memory. Choose this when losing data is unacceptable and unbounded memory use
/// is acceptable. push, emplace, tryPop, and size each take an internal lock; occupancy does not.
///
/// Example:
///
///     UnboundedMpsc<int> queue;
///     queue.push(7);                             // From any producer thread.
///     std::optional<int> item = queue.tryPop();  // From the one consumer thread.
///
/// @tparam ValueType The element type.
///
/// @see MpscQueue, DroppingMpsc, OverwritingMpsc, AnyMpsc
template<typename ValueType>
class UnboundedMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  UnboundedMpsc() = default;
  UnboundedMpsc(const UnboundedMpsc&) = delete;
  UnboundedMpsc(UnboundedMpsc&&) noexcept = delete;
  ~UnboundedMpsc() = default;
  UnboundedMpsc& operator=(const UnboundedMpsc&) = delete;
  UnboundedMpsc& operator=(UnboundedMpsc&&) noexcept = delete;

  /// @brief Enqueue @p value at the back of the queue by move.
  ///
  /// Callable from any producer thread.
  ///
  /// @return Always nullopt; this queue never drops a value.
  std::optional<value_type> push(value_type value)
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);
      mQueue.push_back(std::move(value));
      return std::nullopt;
    }();
  }

  /// @brief Construct a value in place at the back of the queue from @p args, avoiding a move.
  ///
  /// Callable from any producer thread.
  ///
  /// @tparam Args The constructor argument types; value_type must be constructible from them.
  ///
  /// @param args The arguments forwarded to the value_type constructor.
  ///
  /// @return Always nullopt; this queue never drops a value.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);
      mQueue.emplace_back(std::forward<Args>(args)...);
      return std::nullopt;
    }();
  }

  /// @brief Remove and return the oldest queued value.
  ///
  /// Call only from the single consumer thread.
  ///
  /// @return The oldest value, or nullopt when the queue is empty.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);
      if(mQueue.empty())
      {
        return std::nullopt;
      }

      auto value = std::optional<value_type>(std::move(mQueue.front()));
      mQueue.pop_front();
      return value;
    }();
  }

  /// @brief Report the current number of queued values.
  ///
  /// A momentary snapshot; racy under concurrent producers, so use it for metrics, not control
  /// flow.
  [[nodiscard]] size_type size() const
  {
    return [&]() -> size_type
    {
      const auto lock = std::scoped_lock(mMutex);
      return mQueue.size();
    }();
  }

  /// @brief Report the fraction of capacity currently filled, on a scale from 0.0 to 1.0.
  ///
  /// @return Always 0.0; an unbounded queue has no capacity and never approaches dropping.
  [[nodiscard]] double occupancy() const
  {
    return 0.0;
  }

private:
  /// Guards every access to mQueue so producers and the consumer never touch it at once. Mutable so
  /// that the const size and occupancy methods may lock it.
  mutable std::mutex mMutex;

  /// The underlying storage; holds the queued values in arrival order, oldest at the front. Access
  /// it only while holding mMutex.
  std::deque<value_type> mQueue;
};

} // namespace nioc::concurrent
