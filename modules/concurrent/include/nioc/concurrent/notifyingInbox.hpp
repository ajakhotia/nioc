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

/// @brief Wraps an @ref MpscQueue and wakes a single consumer whenever a value is pushed.
///
/// Adds exactly one concern to its queue: the producer-to-consumer wakeup. Every @ref push forwards
/// to the wrapped queue and then fires the notification callback, so a consumer parked after an
/// empty @ref tryPop is roused to drain again.
///
/// Notification is per-push, not coalesced to an empty-to-non-empty edge. Coalescing assumes a
/// single consumer that parks and only needs waking once — an assumption owned by the consumer's
/// scheduler, not by a buffer. So the callback must be idempotent and inexpensive (it may fire
/// while the consumer is already draining), and the consumer must wake on a latched predicate
/// rather than a bare signal, since the callback can run before the consumer parks.
///
/// Itself models @ref MpscQueue, so the notification composes transparently over a concrete queue
/// or over a runtime-selected @ref AnyMpsc.
///
/// @tparam Queue The wrapped queue; must model @ref MpscQueue.
template<MpscQueue Queue>
class NotifyingInbox
{
public:
  using value_type = Queue::value_type;
  using size_type = Queue::size_type;

  /// @brief Constructs the inbox, building the wrapped queue in place.
  ///
  /// @param notify Callback fired after every push to wake the consumer. Maybe empty, in which
  /// case push performs no notification.
  ///
  /// @param queueArgs Arguments forwarded to the wrapped queue's constructor.
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

  /// @brief Enqueues @p value, then fires the notification.
  /// @param value Value to enqueue.
  /// @return The value the wrapped queue sacrificed to stay within capacity, or nullopt.
  std::optional<value_type> push(value_type value)
  {
    auto sacrificed = mQueue.push(std::move(value));
    if(mNotify)
    {
      mNotify();
    }
    return sacrificed;
  }

  /// @brief Constructs a value in place from @p args, then fires the notification.
  /// @param args Constructor arguments for a value_type.
  /// @return The value the wrapped queue sacrificed to stay within capacity, or nullopt.
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
