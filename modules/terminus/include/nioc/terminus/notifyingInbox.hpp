////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/circular_buffer.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <nioc/common/exception.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nioc::terminus
{

/// @brief What a bounded queue does when a push arrives and it is already full.
enum class OverflowPolicy : std::uint8_t
{
  /// @brief Drop the oldest queued item to make room for the new one.
  Overwrite,

  /// @brief Block the pushing thread until a slot frees up.
  Block
};

/// @brief A bounded, thread-safe queue that triggers a callback when work becomes available.
///
/// Producers @ref push from any thread; a single consumer drains with @ref try_pop. The queue fires
/// the notification callback supplied at construction on the empty-to-non-empty transition — i.e.,
/// when a push lands in an empty queue — so a consumer that parked after an empty @ref try_pop is
/// woken exactly when the first new item arrives. The notification callback may run before the
/// consumer parks, so the consumer must wait on a latched predicate rather than a bare signal.
///
/// The @ref OverflowPolicy decides what a push does when the queue is full: @ref
/// OverflowPolicy::Block waits for the consumer to free a slot, @ref OverflowPolicy::Overwrite
/// drops the oldest item. Because @ref try_pop returns the value rather than handing out a
/// reference, it stays safe under both policies and across concurrent producers.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class NotifyingInbox
{
public:
  using Buffer = boost::circular_buffer<ValueType>;
  using value_type = Buffer::value_type;
  using size_type = Buffer::size_type;

  /// @brief Constructs an empty inbox.
  ///
  /// @param capacity Maximum number of items held at once; must be at least 1.
  ///
  /// @param overflowPolicy Behavior of a push that arrives while the inbox is full.
  ///
  /// @param notify Callback fired on the empty-to-non-empty transition. Maybe empty.
  ///
  /// @throws std::invalid_argument If @p capacity is 0.
  NotifyingInbox(
      const size_type capacity,
      const OverflowPolicy overflowPolicy,
      std::function<void()> notify):
      mBuffer(capacity),
      mOverflowPolicy{ overflowPolicy },
      mNotify(std::move(notify))
  {
    if(capacity < 1)
    {
      common::throwException<std::invalid_argument>("NotifyingInbox capacity must be at least 1.");
    }
  }

  NotifyingInbox(const NotifyingInbox&) = delete;
  NotifyingInbox(NotifyingInbox&&) noexcept = delete;
  ~NotifyingInbox() = default;
  NotifyingInbox& operator=(const NotifyingInbox&) = delete;
  NotifyingInbox& operator=(NotifyingInbox&&) noexcept = delete;

  /// @brief Constructs a value in place at the back of the inbox, applying the overflow policy if
  /// the inbox is full.
  ///
  /// Under @ref OverflowPolicy::Block this waits until the consumer frees a slot; under @ref
  /// OverflowPolicy::Overwrite it drops the oldest queued value. Fires the notification callback
  /// when the value lands in an otherwise-empty inbox.
  ///
  /// @param args Constructor arguments forwarded to build the enqueued value in place.
  template<typename... Args>
  void push_back(Args&&... args)
  {
    const auto shouldNotify = [&]()
    {
      auto lock = std::unique_lock(mMutex);

      if(mBuffer.full() and mOverflowPolicy == OverflowPolicy::Block)
      {
        mSpaceAvailable.wait(
            lock,
            [this]
            {
              return not mBuffer.full();
            });
      }

      const auto wasEmpty = mBuffer.empty();
      mBuffer.push_back(std::forward<Args>(args)...);
      return wasEmpty;
    }();

    if(shouldNotify and mNotify)
    {
      mNotify();
    }
  }

  /// @brief Removes and returns the oldest value, or nullopt when the inbox is empty.
  ///
  /// @return The popped value, or std::nullopt if there was nothing to pop.
  [[nodiscard]] std::optional<value_type> try_pop()
  {
    const auto lock = std::scoped_lock(mMutex);
    if(mBuffer.empty())
    {
      return std::nullopt;
    }

    auto value = std::optional<value_type>(std::move(mBuffer.front()));
    mBuffer.pop_front();
    mSpaceAvailable.notify_one();
    return value;
  }

  /// @brief Returns whether the inbox currently holds no values.
  [[nodiscard]] bool empty() const
  {
    const auto lock = std::scoped_lock(mMutex);
    return mBuffer.empty();
  }

  /// @brief Returns the number of values currently held.
  [[nodiscard]] size_type size() const
  {
    const auto lock = std::scoped_lock(mMutex);
    return mBuffer.size();
  }

  /// @brief Returns the fixed maximum number of values the inbox can hold.
  [[nodiscard]] size_type capacity() const
  {
    return mCapacity;
  }

private:
  mutable std::mutex mMutex;
  Buffer mBuffer;
  std::condition_variable mSpaceAvailable;
  const size_type mCapacity{ mBuffer.capacity() };
  const OverflowPolicy mOverflowPolicy;
  const std::function<void()> mNotify;
};

} // namespace nioc::terminus
