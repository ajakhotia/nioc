////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <mutex>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/threadedRunner.hpp>
#include <stop_token>
#include <thread>
#include <utility>

namespace nioc::terminus
{

void ThreadedRunner::launch(std::weak_ptr<Routine> routine)
{
  if(const auto locked = routine.lock())
  {
    locked->attachNotifier(makeNotifier());
  }

  mRoutine = std::move(routine);
  mThread = std::jthread(
      [this](std::stop_token stopToken)
      {
        run(std::move(stopToken));
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
  {
    const auto lock = std::scoped_lock(mMutex);
    mReady = true;
  }
  mCondition.notify_one();
}

void ThreadedRunner::run(std::stop_token stopToken)
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
      logger::error("Routine '{}' failed and will stop: {}", routine->name(), exception.what());
      return;
    }

    if(state == Routine::State::Done)
    {
      return;
    }

    if(state == Routine::State::Waiting)
    {
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

} // namespace nioc::terminus
