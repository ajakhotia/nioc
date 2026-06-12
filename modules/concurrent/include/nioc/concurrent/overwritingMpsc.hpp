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

/// @brief A bounded MPSC queue that, when full, evicts its oldest value to admit the newest.
///
/// Latest-wins: a producer never waits and never loses its own value; instead the queue sacrifices
/// the oldest queued value, which @ref push hands back so the caller can account for the loss.
/// Suits a consumer that cares about the freshest data and tolerates gaps — sensor samples, state
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

  /// @brief Constructs an empty queue.
  /// @param capacity Maximum number of values held at once; must be at least 1.
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

  /// @brief Enqueues @p value; if full, evicts and returns the oldest to make room.
  /// @param value Value to enqueue.
  /// @return The evicted oldest value when the queue was full, otherwise nullopt.
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

  /// @brief Constructs a value in place from @p args; if full, evicts and returns the oldest.
  /// @param args Constructor arguments for a value_type.
  /// @return The evicted oldest value when the queue was full, otherwise nullopt.
  /// @note boost::circular_buffer offers no in-buffer construction, so the value is built once from
  /// @p args and then moved into the buffer.
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

  /// @brief Removes and returns the oldest value, or nullopt when empty. Single consumer only.
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

  /// @brief Number of values currently queued. Racy under concurrent producers; for metrics.
  [[nodiscard]] size_type size() const
  {
    return [&]() -> size_type
    {
      const auto lock = std::scoped_lock(mMutex);
      return mBuffer.size();
    }();
  }

  /// @brief Fraction of capacity currently in use, in [0, 1]; 1.0 when full.
  ///
  /// Racy under concurrent producers; a metric for how close the queue is to dropping, not a
  /// control-flow gate.
  [[nodiscard]] double occupancy() const
  {
    return static_cast<double>(size()) / static_cast<double>(mBuffer.capacity());
  }

private:
  mutable std::mutex mMutex;
  boost::circular_buffer<value_type> mBuffer;
};

} // namespace nioc::concurrent
