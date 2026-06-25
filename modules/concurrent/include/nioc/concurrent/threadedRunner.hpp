////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "routine.hpp"
#include "runner.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>

namespace nioc::concurrent
{

/// @brief A Runner that drives one Routine to completion on its own dedicated thread, sleeping that
/// thread whenever the routine has no work.
///
/// The thread loops on Routine::tick: it ticks again at once while the routine returns Continue,
/// parks on a condition variable while it returns Waiting, and exits once it returns Done. A parked
/// thread resumes when wake() fires or when destruction requests it to stop.
///
/// Example:
///
///     auto runner = std::make_shared<ThreadedRunner>();
///     runner->launch(myRoutine); // myRoutine is a std::shared_ptr<Routine>
///     // ... keep both shared owners alive while it should run ...
///     // runner's destructor stops and joins the thread.
///
/// Must be owned through a std::shared_ptr: the trigger installed on the routine captures a weak
/// reference to this runner (see Runner). Non-copyable and non-movable. Drives at most one routine
/// at a time.
///
/// @see Runner, Routine
class ThreadedRunner final: public Runner
{
public:
  ThreadedRunner() = default;

  ThreadedRunner(const ThreadedRunner&) = delete;

  ThreadedRunner(ThreadedRunner&&) noexcept = delete;

  /// @brief Requests the worker thread to stop and joins it, blocking until it exits.
  ~ThreadedRunner() final = default;

  ThreadedRunner& operator=(const ThreadedRunner&) = delete;

  ThreadedRunner& operator=(ThreadedRunner&&) noexcept = delete;

  /// @brief Attaches this runner's wake trigger to @p routine and starts ticking it on a fresh
  /// thread.
  ///
  /// Intended to be called once. Calling it again stops and joins the previous thread, blocking the
  /// caller, before starting the new one.
  ///
  /// @param routine Held weakly. The caller must keep a shared owner alive for as long as it
  /// should run; if it is already expired, the thread starts and exits immediately.
  void launch(std::weak_ptr<Routine> routine) final;

protected:
  /// @brief Wakes the parked worker thread so it ticks the routine again.
  ///
  /// Invoked through the trigger the routine fires when new work arrives. Thread-safe; a wake that
  /// arrives while the thread is still running is latched and honored at the next park, so no wake
  /// is lost.
  void wake() final;

private:
  /// The routine driven by the worker thread, held weakly so the runner never keeps it alive.
  std::weak_ptr<Routine> mRoutine;

  /// Guards mReady and serializes wake() against the worker thread parking on mCondition.
  std::mutex mMutex;

  /// The condition variable the worker thread parks on while the routine reports Waiting.
  std::condition_variable_any mCondition;

  /// The latch set by wake() and cleared by the worker thread, ensuring a wake that arrives before
  /// the thread parks is not lost.
  bool mReady{false};

  /// The worker thread that ticks the routine; joined on destruction.
  std::jthread mThread;

  /// @brief Worker-thread loop that ticks the routine until it reports Done, its owner expires, or
  /// @p stopToken is signalled, parking on the condition variable while the routine reports
  /// Waiting.
  void run(const std::stop_token& stopToken);
};

} // namespace nioc::concurrent
