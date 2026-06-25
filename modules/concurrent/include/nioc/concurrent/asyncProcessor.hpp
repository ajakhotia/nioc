////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "anyMpsc.hpp"
#include "notifyingInbox.hpp"
#include "routine.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <nioc/logger/logger.hpp>
#include <string>
#include <utility>

namespace nioc::concurrent
{

/// @brief A Routine that delivers values from producer threads to one callback, one value at a
/// time, on the runner's thread.
///
/// Producers call push() from any thread; the value is queued, and the driving runner is woken to
/// run the callback once per queued value. This decouples producers from the consumer: push()
/// returns without waiting for the callback. Exactly one runner may drive this routine (single
/// consumer), and the callback runs serially, never overlapping itself.
///
/// Example:
///
///     auto proc = std::make_shared<AsyncProcessor<int>>(
///         "worker",
///         BufferMode::Unbounded,
///         0,
///         [](int v) { handle(v); });
///     auto runner = std::make_shared<ThreadedRunner>();
///     runner->launch(proc); // drives proc on its own thread; both must stay alive
///     proc->push(42);       // from any producer thread
///
/// @tparam ValueType The value handed from producers to the callback. Moved through the queue, so
/// it must be movable.
///
/// @see Routine, Runner, NotifyingInbox, BufferMode
template<typename ValueType>
class AsyncProcessor final: public Routine
{
public:
  /// @brief The callback run once per value, on the runner's thread, never overlapping itself.
  ///
  /// An exception thrown out of it is caught and logged, after which the processor stops and any
  /// values still queued are never delivered.
  using Process = std::function<void(ValueType)>;

  /// @brief Build a processor with the given queue policy and callback.
  ///
  /// @param name Label shown for this routine in log messages.
  ///
  /// @param bufferMode BufferMode::Overwriting (bounded; drops the oldest value when full) or
  /// BufferMode::Unbounded (grows without limit).
  ///
  /// @param capacity Queue size for Overwriting mode; ignored when Unbounded. Must be >= 1 for
  /// Overwriting.
  ///
  /// @param process The per-value callback. Stored by move; any state it captures must outlive
  /// this object.
  ///
  /// @throws std::invalid_argument When @p bufferMode is Overwriting and @p capacity is 0.
  AsyncProcessor(
      std::string name,
      const BufferMode bufferMode,
      const std::size_t capacity,
      Process process):
    Routine(std::move(name)),
    mProcess(std::move(process)),
    mInbox([this] { triggerRunner(); }, bufferMode, capacity)
  {
  }

  AsyncProcessor(const AsyncProcessor&) = delete;
  AsyncProcessor(AsyncProcessor&&) noexcept = delete;
  ~AsyncProcessor() final = default;
  AsyncProcessor& operator=(const AsyncProcessor&) = delete;
  AsyncProcessor& operator=(AsyncProcessor&&) noexcept = delete;

  /// @brief Queue a value for the callback and wake the runner.
  ///
  /// Thread-safe; call from any producer thread. Returns without waiting for the callback. In
  /// Overwriting mode, a full queue silently drops the oldest value to make room.
  void push(ValueType value)
  {
    mInbox.push(std::move(value));
  }

private:
  /// The callback invoked for each dequeued value. Owned by this object for its whole lifetime.
  Process mProcess;

  /// The queue that holds pushed values and wakes the runner when one arrives. Producers feed it
  /// through push(); step() drains it one value at a time.
  NotifyingInbox<AnyMpsc<ValueType>> mInbox;

  /// @brief Run one unit of work for the runner: pop at most one value and pass it to the callback.
  ///
  /// Called repeatedly by the driving runner on its own thread, never concurrently with itself.
  /// Returns State::Continue after delivering a value (more may be queued), State::Waiting when the
  /// queue is empty, and State::Done when the callback throws, which stops the processor and leaves
  /// any remaining values undelivered. Any exception from the callback is caught and logged here,
  /// so this never propagates one.
  ///
  /// @return The runner state that decides whether to keep stepping, wait, or stop.
  [[nodiscard]] State step() noexcept final
  {
    try
    {
      auto value = mInbox.tryPop();
      if(not value)
      {
        return State::Waiting;
      }

      mProcess(std::move(*value));
      return State::Continue;
    }
    catch(const std::exception& exception)
    {
      logger::error("[{}] {}", name(), exception.what());
    }
    catch(...)
    {
      logger::error("[{}] unhandled exception", name());
    }

    return State::Done;
  }
};

} // namespace nioc::concurrent
