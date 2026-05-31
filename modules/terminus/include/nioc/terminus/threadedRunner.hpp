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

namespace nioc::terminus
{

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
  void run(std::stop_token stopToken);

  std::weak_ptr<Routine> mRoutine;
  std::mutex mMutex;
  std::condition_variable_any mCondition;
  bool mReady{ false };
  std::jthread mThread;
};

} // namespace nioc::terminus
