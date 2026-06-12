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

/// @brief Runs a user callback on a background thread when a registered process signal arrives.
///
/// Maps each signal number to an action. On construction, it installs a handler for every signal in
/// the map and starts one watcher thread. When a registered signal arrives, the watcher invokes
/// that signal's action and passes the number of times the signal has been received so far (1 on
/// the first delivery, 2 on the second, and so on). An action can use that count to escalate — for
/// example, ask for a clean shutdown on the first interrupt and exit immediately on the second.
///
/// Actions run on the watcher thread, not inside the OS signal handler, so an action may safely do
/// work that is not async-signal-safe, such as logging, locking, or allocating. The watcher runs
/// actions one at a time, in the order their signals arrive.
///
/// The handlers stay installed for the lifetime of the catcher: the destructor restores the default
/// behavior of each registered signal and joins the watcher thread. Construct one instance and keep
/// it alive for as long as the signals should be handled. Signal handlers are process-global, so do
/// not register the same signal with more than one catcher at a time.
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
  /// @param count Number of times this signal has been received so far (1 on the first delivery).
  using SignalAction = std::function<void(std::int32_t)>;

  /// @brief Installs a handler for each signal and starts the watcher thread.
  ///
  /// @tparam Args Types of the `{signal number, SignalAction}` pairs; forwarded to build the
  /// signal-to-action map.
  ///
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
