////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/circular_buffer.hpp>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <nioc/common/exception.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nioc::concurrent
{

/// @brief Bounded multi-producer, single-consumer queue. When full, drops the oldest value to fit
/// the newest.
///
/// A producer never blocks and never loses its own value. When the queue is full, @ref push and
/// @ref emplace drop the oldest value and return it, so the caller can count what was lost. Use it
/// when the consumer wants the freshest data and can tolerate gaps, such as sensor samples or state
/// updates.
///
/// Models @ref MpscQueue. See it for the producer/consumer contract.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class OverwritingMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  /// @brief Builds an empty queue.
  /// @param capacity Maximum number of values held at once. Must be at least 1.
  /// @throws std::invalid_argument If @p capacity is 0.
  explicit OverwritingMpsc(const size_type capacity): mBuffer(capacity)
  {
    if(capacity < 1)
    {
      common::throwException<std::invalid_argument>("OverwritingMpsc capacity must be at least 1.");
    }
  }

  OverwritingMpsc(const OverwritingMpsc&) = delete;
  OverwritingMpsc(OverwritingMpsc&&) noexcept = delete;
  ~OverwritingMpsc() = default;
  OverwritingMpsc& operator=(const OverwritingMpsc&) = delete;
  OverwritingMpsc& operator=(OverwritingMpsc&&) noexcept = delete;

  /// @brief Adds @p value. If full, drops the oldest value and returns it.
  /// @param value Value to add.
  /// @return The dropped oldest value if the queue was full, otherwise nullopt.
  std::optional<value_type> push(value_type value)
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);

      // On a full buffer, push_back overwrites the oldest slot and rotates, so capture it first.
      auto evicted = mBuffer.full() ? std::optional<value_type>(std::move(mBuffer.front()))
                                    : std::nullopt;
      mBuffer.push_back(std::move(value));
      return evicted;
    }();
  }

  /// @brief Builds a value from @p args and adds it. If full, drops the oldest value and returns
  /// it.
  /// @param args Constructor arguments for a value_type.
  /// @return The dropped oldest value if the queue was full, otherwise nullopt.
  /// @note The value is built once, then moved into the queue (not constructed in place).
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);

      // On a full buffer, push_back overwrites the oldest slot and rotates, so capture it first.
      auto evicted = mBuffer.full() ? std::optional<value_type>(std::move(mBuffer.front()))
                                    : std::nullopt;
      mBuffer.push_back(value_type(std::forward<Args>(args)...));
      return evicted;
    }();
  }

  /// @brief Removes and returns the oldest value, or nullopt if empty. Single consumer only.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return [&]() -> std::optional<value_type>
    {
      const auto lock = std::scoped_lock(mMutex);
      if(mBuffer.empty())
      {
        return std::nullopt;
      }

      auto value = std::optional<value_type>(std::move(mBuffer.front()));
      mBuffer.pop_front();
      return value;
    }();
  }

  /// @brief Number of values currently queued. Stale once other threads push or pop; use for
  /// metrics only.
  [[nodiscard]] size_type size() const
  {
    return [&]() -> size_type
    {
      const auto lock = std::scoped_lock(mMutex);
      return mBuffer.size();
    }();
  }

  /// @brief Fraction of capacity in use, in [0, 1]; 1.0 when full.
  ///
  /// Stale once other threads push or pop. Use for metrics only, not to decide control flow.
  [[nodiscard]] double occupancy() const
  {
    return static_cast<double>(size()) / static_cast<double>(mBuffer.capacity());
  }

private:
  mutable std::mutex mMutex;
  boost::circular_buffer<value_type> mBuffer;
};

} // namespace nioc::concurrent
