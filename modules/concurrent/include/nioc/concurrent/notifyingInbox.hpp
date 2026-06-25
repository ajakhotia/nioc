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

/// @brief An MpscQueue wrapper that calls a notify callback after every enqueue, so a waiting
/// consumer learns that work is ready.
///
/// Use it to decouple producers from how the consumer is scheduled (a runner step, a condition
/// variable, an event loop). Producers call push or emplace; the inbox then fires the callback.
///
/// Example:
///
///     std::condition_variable cv;
///     NotifyingInbox<MyQueue> inbox{[&] { cv.notify_one(); }, queueCapacity};
///     inbox.push(job); // enqueues, then wakes the consumer
///
/// Concurrency follows the wrapped queue: many producers may enqueue at once, but only one consumer
/// may call tryPop. The callback runs synchronously on the enqueuing thread, so keep it cheap and
/// thread-safe. The notify fires on every enqueue, including ones that drop or overwrite a value.
///
/// Not copyable and not movable.
///
/// @tparam Queue The wrapped queue type; must model MpscQueue.
///
/// @see MpscQueue
template<MpscQueue Queue>
class NotifyingInbox
{
public:
  using value_type = Queue::value_type;
  using size_type = Queue::size_type;

  /// @brief Builds the inbox, forwarding @p queueArgs to the wrapped queue's constructor.
  ///
  /// @tparam QueueArgs Types of the trailing arguments forwarded to the wrapped queue's
  /// constructor.
  ///
  /// @param notify Called after each push or emplace. Pass an empty function to disable
  /// notification. Stored and invoked for the inbox's whole lifetime, so any
  /// state it captures must outlive the inbox.
  ///
  /// @param queueArgs Forwarded to the wrapped queue's constructor (for example, the capacity).
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

  /// @brief Enqueues @p value by move, then notifies.
  ///
  /// @return The value the queue dropped or evicted to make room, or nullopt if none was
  /// sacrificed. A bounded queue at capacity may sacrifice a value; an unbounded one never
  /// does. The notify fires either way.
  std::optional<value_type> push(value_type value)
  {
    auto sacrificed = mQueue.push(std::move(value));
    if(mNotify)
    {
      mNotify();
    }
    return sacrificed;
  }

  /// @brief Constructs a value in place from @p args, then notifies.
  ///
  /// @tparam Args Types of the arguments forwarded to a value_type constructor.
  ///
  /// @param args Forwarded to a value_type constructor.
  ///
  /// @return The value the queue dropped or evicted to make room, or nullopt if none was
  /// sacrificed. The notify fires either way.
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

  /// @brief Removes and returns the oldest value, or nullopt when empty. Does not notify.
  ///
  /// Call from the single consumer thread only.
  [[nodiscard]] std::optional<value_type> tryPop()
  {
    return mQueue.tryPop();
  }

  /// @brief Current number of queued values. Racy under concurrent producers; use for metrics, not
  /// control flow.
  [[nodiscard]] size_type size() const
  {
    return mQueue.size();
  }

  /// @brief The fraction of capacity in use, in [0, 1]; 1.0 when full, 0.0 for an unbounded queue.
  /// Racy; use for metrics, not control flow.
  [[nodiscard]] double occupancy() const
  {
    return mQueue.occupancy();
  }

private:
  /// The callback run synchronously on the enqueuing thread after each push or emplace. Empty when
  /// notification is disabled.
  const std::function<void()> mNotify;

  /// The wrapped queue that holds the values and provides the concurrency: many producers may
  /// enqueue at once, but only one consumer may pop.
  Queue mQueue;
};

} // namespace nioc::concurrent
