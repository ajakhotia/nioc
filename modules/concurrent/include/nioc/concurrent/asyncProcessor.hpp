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

/// @brief A @ref Routine that runs a callback on each value pushed to its inbox.
///
/// Producers call @ref push from any thread. The callback runs one value at a time on the Runner's
/// thread, so it never runs concurrently with itself. When the inbox is empty a step reports @ref
/// State::Waiting, and the next @ref push wakes the Runner to drain it. The @ref BufferMode picked
/// at construction decides whether a full bounded inbox drops a value or the inbox grows without
/// limit.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class AsyncProcessor final: public Routine
{
public:
  /// @brief Callback that handles one value, run on the Runner's thread.
  using Process = std::function<void(ValueType)>;

  /// @brief Builds the processor.
  ///
  /// @param name Name for this processor (see @ref Routine::name).
  ///
  /// @param bufferMode How the inbox stores values (see @ref BufferMode).
  ///
  /// @param capacity Inbox capacity for a bounded @p bufferMode; ignored when unbounded.
  ///
  /// @param process Callback run on the Runner's thread for each value.
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

  /// @brief Queues a value for the processor to handle, and wakes the Runner.
  ///
  /// Thread-safe; call from any thread. Never blocks. A full bounded inbox drops a value per its
  /// @ref BufferMode.
  ///
  /// @param value Value to queue.
  void push(ValueType value)
  {
    mInbox.push(std::move(value));
  }

private:
  Process mProcess;
  NotifyingInbox<AnyMpsc<ValueType>> mInbox;

  /// @brief Pops one value and runs the callback on it.
  ///
  /// Catches and logs any exception the callback throws. Never throws.
  ///
  /// @return @ref State::Continue after handling a value, @ref State::Waiting when the inbox is
  /// empty, or @ref State::Done if the callback threw.
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
