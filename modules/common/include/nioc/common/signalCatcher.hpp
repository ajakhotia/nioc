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

/// @brief Runs a callback on a background thread when a registered signal arrives.
///
/// Give it a map of signal number to action. It installs a handler for each signal and starts one
/// watcher thread. When a registered signal arrives, the watcher calls that signal's action and
/// passes how many times the signal has been received so far (1 on the first, 2 on the second,
/// ...). An action can use the count to escalate, e.g. ask for a clean shutdown on the first
/// interrupt and exit on the second.
///
/// Actions run on the watcher thread, not in the OS signal handler, so they may do work that is not
/// async-signal-safe (logging, locking, allocating). The watcher runs actions one at a time.
///
/// Signals coalesce: only one delivery can be pending. If another signal arrives before the watcher
/// handles the pending one, it replaces it and the earlier delivery is lost.
///
/// Keep the instance alive for as long as you want signals handled. The destructor restores each
/// signal's default behavior and joins the watcher thread. The pending slot and stop signal are
/// process-global, so create at most one SignalCatcher per process.
///
/// @code
/// auto stopSource = std::stop_source{};
///
/// // First Ctrl-C asks for a clean shutdown; a second one exits immediately.
/// auto catcher = nioc::common::SignalCatcher{
///     std::pair{SIGINT, [&stopSource](const std::int32_t count)
///                       {
///                         if(count == 1)
///                         {
///                           stopSource.request_stop();
///                         }
///                         else
///                         {
///                           std::_Exit(130);
///                         }
///                       }}};
/// @endcode
class SignalCatcher
{
public:
  /// @brief Callback run when a registered signal arrives.
  /// @param count Times this signal has been received so far (1 on the first delivery).
  using SignalAction = std::function<void(std::int32_t)>;

  /// @brief Installs a handler for each signal and starts the watcher thread.
  ///
  /// @tparam Args Types of the `{signal number, SignalAction}` pairs.
  /// @param args One `{signal, action}` pair per signal to handle.
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

  /// @brief Restores the default behavior of each registered signal and stops the watcher thread.
  ~SignalCatcher();

  SignalCatcher& operator=(const SignalCatcher&) = delete;

  SignalCatcher& operator=(SignalCatcher&&) noexcept = delete;

private:
  const std::unordered_map<std::int32_t, SignalAction> mSignalActions;
  std::jthread mWatchThread;
  std::unordered_map<std::int32_t, std::int32_t> mSignalCounter;

  /// @brief Watcher-thread loop: waits for a delivered signal and runs its action with the count.
  void watch();

  /// @brief Installs the process handler that records @p signal for the watcher.
  static void setupHandler(int signal);
};

} // namespace nioc::common
