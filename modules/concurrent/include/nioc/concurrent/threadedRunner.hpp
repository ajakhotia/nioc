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

/// @brief A @ref Runner that drives its routine's step loop on a dedicated thread.
///
/// @ref launch spawns one `std::jthread` that calls @ref Routine::step in a loop. A @ref
/// State::Waiting return parks the thread on a condition variable; the routine's trigger (or @ref
/// requestStop) wakes it. The loop ends, and the thread finishes, when the routine reports @ref
/// State::Done, throws (the error is logged), or expires. A thrown exception stops only this
/// routine, not the process.
class ThreadedRunner final: public Runner
{
public:
  ThreadedRunner() = default;

  ThreadedRunner(const ThreadedRunner&) = delete;

  ThreadedRunner(ThreadedRunner&&) noexcept = delete;

  ~ThreadedRunner() final = default;

  ThreadedRunner& operator=(const ThreadedRunner&) = delete;

  ThreadedRunner& operator=(ThreadedRunner&&) noexcept = delete;

  void launch(std::weak_ptr<Routine> routine) final;

  void waitUntilStopped() final;

  void requestStop() noexcept final;

protected:
  void wake() final;

private:
  void run(const std::stop_token& stopToken);

  std::weak_ptr<Routine> mRoutine;
  std::mutex mMutex;
  std::condition_variable_any mCondition;
  bool mReady{false};
  std::jthread mThread;
};

} // namespace nioc::concurrent
