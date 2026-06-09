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

/// @brief A @ref Routine that drains an inbox, handing each value to a processing callback.
///
/// Producers call @ref push from any thread; the Routine's @ref step pops one value at a time and
/// runs the callback on the Runner's thread, so the callback never runs concurrently with itself.
/// When the inbox empties, a step reports @ref State::Waiting and the inbox reschedules the Routine
/// once the next value arrives. The @ref BufferMode chosen at construction decides whether a full
/// bounded inbox sacrifices a value or the inbox grows unbounded.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class AsyncProcessor final: public Routine
{
public:
  /// @brief Callback that handles one dequeued value, run on the Runner's thread.
  using Process = std::function<void(ValueType)>;

  /// @brief Constructs the processor with its inbox and the callback that handles each value.
  ///
  /// @param name Human-readable identity for this processor (see @ref Routine::name).
  ///
  /// @param bufferMode Storage discipline of the inbox (see @ref BufferMode).
  ///
  /// @param capacity Inbox capacity for a bounded @p bufferMode; ignored when unbounded.
  ///
  /// @param process Callback run on the Runner's thread for each dequeued value.
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

  /// @brief Enqueues a value for the processor to handle.
  ///
  /// Thread-safe; callable from any producer thread. Never blocks; a full bounded inbox sacrifices
  /// a value per its @ref BufferMode. Wakes the Runner when work arrives.
  ///
  /// @param value Value to enqueue.
  void push(ValueType value)
  {
    mInbox.push(std::move(value));
  }

private:
  Process mProcess;
  NotifyingInbox<AnyMpsc<ValueType>> mInbox;

  /// @brief Pops one queued value and runs the callback on it.
  ///
  /// Catches every exception the callback may throw, logs it, and reports @ref State::Done so a
  /// failing processor winds down gracefully rather than escalating. Never throws.
  ///
  /// @return @ref State::Continue after handling a value, @ref State::Waiting when the inbox is
  /// empty, or @ref State::Done if the callback throws.
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
