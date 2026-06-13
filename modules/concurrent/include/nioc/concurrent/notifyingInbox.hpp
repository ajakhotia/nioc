////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "mpscQueue.hpp"
#include <concepts>
#include <functional>
#include <optional>
#include <utility>

namespace nioc::concurrent
{

/// @brief Wraps an @ref MpscQueue and wakes a single consumer on every push.
///
/// Each @ref push and @ref emplace adds the value, then fires the notification callback. This wakes
/// a consumer that parked after an empty @ref tryPop.
///
/// The callback fires once per push, not once per empty-to-non-empty edge. So:
/// - Make the callback idempotent and cheap. It may fire while the consumer is already draining.
/// - Have the consumer wake on a latched predicate, not a bare signal. The callback can run before
///   the consumer parks.
///
/// This type also models @ref MpscQueue, so it can wrap a concrete queue or a runtime-selected
/// @ref AnyMpsc.
///
/// @tparam Queue The wrapped queue. Must model @ref MpscQueue.
template<MpscQueue Queue>
class NotifyingInbox
{
public:
  using value_type = Queue::value_type;
  using size_type = Queue::size_type;

  /// @brief Builds the wrapped queue in place.
  ///
  /// @param notify Called after every push to wake the consumer. If empty, push does not notify.
  /// @param queueArgs Forwarded to the wrapped queue's constructor.
  template<typename... QueueArgs>
  explicit NotifyingInbox(std::function<void()> notify, QueueArgs&&... queueArgs):
    mNotify(std::move(notify)),
    mQueue(std::forward<QueueArgs>(queueArgs)...)
  {
  }

  NotifyingInbox(const NotifyingInbox&) = delete;
  NotifyingInbox(NotifyingInbox&&) noexcept = delete;
  ~NotifyingInbox() = default;
  NotifyingInbox& operator=(const NotifyingInbox&) = delete;
  NotifyingInbox& operator=(NotifyingInbox&&) noexcept = delete;

  /// @brief Adds @p value, then notifies.
  /// @param value Value to enqueue.
  /// @return The value the queue dropped to stay within capacity, or nullopt.
  std::optional<value_type> push(value_type value)
  {
    auto sacrificed = mQueue.push(std::move(value));
    if(mNotify)
    {
      mNotify();
    }
    return sacrificed;
  }

  /// @brief Builds a value_type in place from @p args, then notifies.
  /// @param args Constructor arguments for a value_type.
  /// @return The value the queue dropped to stay within capacity, or nullopt.
  template<typename... Args>
    requires std::constructible_from<value_type, Args...>
  std::optional<value_type> emplace(Args&&... args)
  {
    auto sacrificed = mQueue.emplace(std::forward<Args>(args)...);
    if(mNotify)
    {
      mNotify();
    }
    return sacrificed;
  }

  /// @brief Removes and returns the oldest value, or nullopt when empty. Single consumer only.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return mQueue.tryPop();
  }

  /// @brief Number of values currently queued. Racy; for metrics.
  [[nodiscard]] size_type size() const
  {
    return mQueue.size();
  }

  /// @brief Fraction of capacity in use, in [0, 1]; 0.0 for unbounded storage. Racy; for metrics.
  [[nodiscard]] double occupancy() const
  {
    return mQueue.occupancy();
  }

private:
  const std::function<void()> mNotify;
  Queue mQueue;
};

} // namespace nioc::concurrent
