////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <functional>
#include <ranges>
#include <thread>

namespace nioc::common
{

/// @brief A guard object that installs process signal handlers for its lifetime and runs your
/// per-signal callbacks on a dedicated background thread instead of inside the OS signal handler.
///
/// Build it with a map from signal number to action. While it is alive, delivery of any registered
/// signal wakes an internal watcher thread, which then calls that signal's action. Because the
/// action runs on a normal thread (not in the async-signal context), it may use any code, including
/// locks, allocation, and I/O. Handlers are installed at construction and reset to the OS default
/// at destruction.
///
/// Example:
///
///     // Catch SIGINT and SIGTERM; print and request shutdown.
///     nioc::common::SignalCatcher catcher{{
///         {SIGINT, [&](std::int32_t n) { std::cout << "interrupted " << n << " times\n"; }},
///         {SIGTERM, [&](std::int32_t) { shutdown(); }},
///     }};
///
/// Gotchas:
/// - Process-global. All instances share one signal slot and one handler, so only ONE may be alive
///   at a time. A second live instance corrupts both dispatch and the shutdown handshake.
/// - Lossy. Deliveries pass through a single value slot, so rapid or interleaved signals may
///   coalesce; not every delivery produces an action call.
/// - Actions are serialized on the watcher thread and block its loop, so keep them short.
/// - Non-copyable and non-movable.
class SignalCatcher
{
public:
  /// @brief Callback invoked once per caught delivery of a registered signal.
  ///
  /// Its single argument is the running count of how many times this instance has caught that
  /// specific signal (1 on the first delivery). This is a count, not the signal number.
  using SignalAction = std::function<void(std::int32_t)>;

  /// @brief Register signal/action pairs, install the handlers, and start the watcher thread.
  ///
  /// Catching begins immediately. Pass at most one live instance per process.
  ///
  /// @tparam Args Forwarded to construct an `unordered_map<std::int32_t, SignalAction>`; typically
  /// a single brace-enclosed list of `{signalNumber, action}` pairs.
  ///
  /// @param args The `{signalNumber, action}` pairs to register, one per signal to catch.
  template<typename... Args>
  explicit SignalCatcher(Args&&... args):
    mSignalActions{std::forward<Args>(args)...},
    mWatchThread([this]() { watch(); })
  {
    for(const auto signal: mSignalActions | std::views::keys)
    {
      setupHandler(signal);
    }
  }

  SignalCatcher(const SignalCatcher&) = delete;

  SignalCatcher(SignalCatcher&&) noexcept = delete;

  /// @brief Restore the OS default disposition for every registered signal and stop the watcher
  /// thread.
  ///
  /// Blocks until the watcher thread joins; an action already in progress runs to completion.
  ~SignalCatcher();

  SignalCatcher& operator=(const SignalCatcher&) = delete;

  SignalCatcher& operator=(SignalCatcher&&) noexcept = delete;

private:
  /// The action to run for each registered signal, keyed by signal number. Fixed at construction.
  const std::unordered_map<std::int32_t, SignalAction> mSignalActions;

  /// The background thread that waits for a delivered signal and runs its action. Joined on
  /// destruction.
  std::jthread mWatchThread;

  /// Running count of deliveries caught per signal number, passed to each action. Owned and updated
  /// only by the watcher thread.
  std::unordered_map<std::int32_t, std::int32_t> mSignalCounter;

  /// @brief Loop on the watcher thread: block until a registered signal is delivered, then
  /// increment its count and invoke its action. Returns once the thread is asked to stop during
  /// destruction.
  void watch();

  /// @brief Install this class's handler for one signal so its delivery wakes the watcher thread.
  ///
  /// @param signal The signal number whose OS disposition is replaced.
  static void setupHandler(std::int32_t signal);
};

} // namespace nioc::common
