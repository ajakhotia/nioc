////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "notifyingInbox.hpp"
#include "routine.hpp"
#include <cstddef>
#include <functional>
#include <string_view>
#include <utility>

namespace nioc::terminus
{

/// @brief A Routine that drains a bounded inbox, handing each value to a processing callback.
///
/// Producers call @ref push_back from any thread; the Routine's @ref step pops one value at a time
/// and runs the callback on the Runner's thread. When the inbox empties, step reports @ref
/// State::Waiting; the inbox reschedules the Routine when the next value arrives. The @ref
/// OverflowPolicy governs a push that arrives while the inbox is full.
///
/// @tparam ValueType Type of the queued values.
template<typename ValueType>
class AsyncProcessor final: public Routine
{
public:
  using Process = std::function<void(ValueType)>;

  /// @brief Constructs the processor with a bounded inbox and the callback that handles each value.
  ///
  /// @param capacity Maximum number of queued values held at once; must be at least 1.
  ///
  /// @param overflowPolicy Behavior of a @ref push that arrives while the inbox is full.
  ///
  /// @param process Callback run on the Runner's thread for each dequeued value.
  AsyncProcessor(const std::size_t capacity, const OverflowPolicy overflowPolicy, Process process):
      mProcess(std::move(process)),
      mInbox(
          capacity,
          overflowPolicy,
          [this]
          {
            notifyReady();
          })
  {
  }

  AsyncProcessor(const AsyncProcessor&) = delete;
  AsyncProcessor(AsyncProcessor&&) noexcept = delete;
  ~AsyncProcessor() final = default;
  AsyncProcessor& operator=(const AsyncProcessor&) = delete;
  AsyncProcessor& operator=(AsyncProcessor&&) noexcept = delete;

  /// @brief Enqueues a value, constructed in place, for the processor to handle.
  ///
  /// Thread-safe; callable from any producer thread. Applies the @ref OverflowPolicy if the inbox
  /// is full, and wakes the Runner when the value lands in an otherwise-empty inbox.
  ///
  /// @param args Constructor arguments forwarded to build the queued value in place.
  template<typename... Args>
  void push_back(Args&&... args)
  {
    mInbox.push_back(std::forward<Args>(args)...);
  }

  [[nodiscard]] State step() final
  {
    auto value = mInbox.try_pop();
    if(not value)
    {
      return State::Waiting;
    }

    mProcess(std::move(*value));
    return State::Continue;
  }

  [[nodiscard]] std::string_view name() const final
  {
    return "AsyncProcessor";
  }

private:
  Process mProcess;
  NotifyingInbox<ValueType> mInbox;
};

} // namespace nioc::terminus
