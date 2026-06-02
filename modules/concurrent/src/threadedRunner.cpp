////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <mutex>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/logger/logger.hpp>
#include <stop_token>
#include <thread>
#include <utility>

namespace nioc::concurrent
{

void ThreadedRunner::launch(std::weak_ptr<Routine> routine)
{
  if(const auto locked = routine.lock())
  {
    locked->attachNotifier(makeNotifier());
    logger::debug("[{}] launching", locked->name());
  }

  mRoutine = std::move(routine);
  mThread = std::jthread(
      [this](const std::stop_token& stopToken)
      {
        run(stopToken);
      });
}

void ThreadedRunner::waitUntilStopped()
{
  if(mThread.joinable())
  {
    mThread.join();
  }
}

void ThreadedRunner::requestStop() noexcept
{
  mThread.request_stop();
}

void ThreadedRunner::wake()
{
  logger::trace("wake requested");
  {
    const auto lock = std::scoped_lock(mMutex);
    mReady = true;
  }
  mCondition.notify_one();
}

void ThreadedRunner::run(const std::stop_token& stopToken)
{
  while(not stopToken.stop_requested())
  {
    auto routine = mRoutine.lock();
    if(not routine)
    {
      return;
    }

    auto state = Routine::State::Done;
    try
    {
      state = routine->step();
    }
    catch(const std::exception& exception)
    {
      logger::error("[{}] step failed and will stop: {}", routine->name(), exception.what());
      return;
    }

    if(state == Routine::State::Done)
    {
      logger::debug("[{}] finished (Done)", routine->name());
      return;
    }

    if(state == Routine::State::Waiting)
    {
      logger::trace("[{}] waiting; parking until notified", routine->name());
      routine.reset();

      auto lock = std::unique_lock(mMutex);
      mCondition.wait(
          lock,
          stopToken,
          [this]
          {
            return mReady;
          });
      mReady = false;
    }
  }
}

} // namespace nioc::concurrent
