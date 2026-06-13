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

/// @brief An unbounded MPSC queue. Keeps every pushed value; never drops any.
///
/// Producers never wait and the queue never discards, so @ref push always returns nullopt. The cost
/// is memory: if the consumer is slower than the producers, the queue grows without limit. Use it
/// for offline or replay work where keeping every value matters more than bounding memory.
///
/// Models @ref MpscQueue. See it for the producer/consumer contract.
///
/// @tparam ValueType Type of the queued values.
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

  /// @brief Adds @p value to the queue. Always returns nullopt; nothing is ever dropped.
  /// @param value Value to add.
  std::optional<value_type> push(value_type value)
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);
      mQueue.push_back(std::move(value));
      return std::nullopt;
    }();
  }

  /// @brief Builds a value in place from @p args. Always returns nullopt; nothing is ever dropped.
  /// @param args Constructor arguments for a value_type.
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

  /// @brief Removes and returns the oldest value, or nullopt if empty. Call from one consumer only.
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

  /// @brief Number of values in the queue right now. Racy; use it as a backlog signal only.
  [[nodiscard]] size_type size() const
  {
    return [&]() -> size_type
    {
      const auto lock = std::scoped_lock(mMutex);
      return mQueue.size();
    }();
  }

  /// @brief Always 0.0: an unbounded queue has no capacity, so it can never fill up.
  [[nodiscard]] double occupancy() const
  {
    return 0.0;
  }

private:
  mutable std::mutex mMutex;
  std::deque<value_type> mQueue;
};

} // namespace nioc::concurrent
