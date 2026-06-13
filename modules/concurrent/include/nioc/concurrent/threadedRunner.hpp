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

/// @brief A @ref Runner that drives its routine's step loop on one dedicated thread.
///
/// @ref launch starts a thread that calls @ref Routine::step in a loop. On @ref State::Waiting the
/// thread sleeps until the routine's trigger wakes it. The loop ends, and the thread stops, when
/// the routine reports @ref State::Done, expires, or the Runner is destroyed. Destruction joins the
/// thread. @ref Routine::step never throws; a failing routine reports Done itself.
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

protected:
  void wake() final;

private:
  std::weak_ptr<Routine> mRoutine;
  std::mutex mMutex;
  std::condition_variable_any mCondition;
  bool mReady{false};
  std::jthread mThread;

  void run(const std::stop_token& stopToken);
};

} // namespace nioc::concurrent
