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

/// @brief A bounded, thread-safe FIFO queue for many producers and one consumer that drops its
/// oldest element when full instead of blocking or failing.
///
/// Models @ref MpscQueue. Pushing into a full queue evicts the oldest element and returns it to the
/// caller, so the queue always keeps the newest @c capacity elements. Use it when stale data is
/// worthless and the freshest samples must win, such as sensor frames. Every method is safe to call
/// concurrently from any thread; the queue is neither copyable nor movable.
///
/// Example:
///
///     auto queue = OverwritingMpsc<Frame>(8);
///     queue.push(frame);              // producer thread(s)
///     auto next = queue.tryPop();     // consumer thread
///
/// @tparam ValueType The element type. Must be movable.
///
/// @see MpscQueue, DroppingMpsc, UnboundedMpsc
template<typename ValueType>
class OverwritingMpsc
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;

  /// @brief Construct a queue that holds at most @p capacity elements.
  ///
  /// @param capacity Maximum number of elements retained. Must be at least 1.
  ///
  /// @throws std::invalid_argument if @p capacity is 0.
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

  /// @brief Append @p value to the back of the queue.
  ///
  /// @return The evicted oldest element if the queue was full before the call, else @c nullopt.
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

  /// @brief Construct an element in place from @p args and append it to the back of the queue.
  ///
  /// @tparam Args Argument types; @c value_type must be constructible from them.
  ///
  /// @param args Arguments forwarded to the @c value_type constructor.
  ///
  /// @return The evicted oldest element if the queue was full before the call, else @c nullopt.
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

  /// @brief Remove and return the oldest element, or @c nullopt if the queue is empty.
  ///
  /// Call this only from the single consumer thread.
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

  /// @brief Return the current element count.
  ///
  /// The result is a momentary snapshot and may be stale once it returns.
  [[nodiscard]] size_type size() const
  {
    return [&]() -> size_type
    {
      const auto lock = std::scoped_lock(mMutex);
      return mBuffer.size();
    }();
  }

  /// @brief Return the current fraction filled, size divided by capacity, in the range [0, 1].
  ///
  /// The result is a momentary snapshot and may be stale once it returns.
  [[nodiscard]] double occupancy() const
  {
    return static_cast<double>(size()) / static_cast<double>(mBuffer.capacity());
  }

private:
  /// Guards every access to mBuffer so that producers and the consumer never race. Marked mutable
  /// so const observers such as size() and occupancy() can lock it.
  mutable std::mutex mMutex;

  /// Fixed-capacity ring that retains the newest @c capacity elements; a push into a full ring
  /// overwrites the oldest slot. Access it only while holding mMutex.
  boost::circular_buffer<value_type> mBuffer;
};

} // namespace nioc::concurrent
