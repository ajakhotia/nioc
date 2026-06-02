////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/concurrent/routine.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <stdexcept>
#include <thread>

namespace nioc::concurrent
{
namespace
{

using namespace std::chrono_literals;

/// Returns Done after a fixed number of iterations, counting how many times it ran.
class CountingRoutine final: public Routine
{
public:
  explicit CountingRoutine(const int doneAfter): Routine("CountingRoutine"), mDoneAfter{ doneAfter }
  {
  }

  State step() final
  {
    if(++mIterations >= mDoneAfter)
    {
      return State::Done;
    }
    return State::Continue;
  }

  [[nodiscard]] int iterations() const
  {
    return mIterations.load();
  }

private:
  std::atomic<int> mIterations{ 0 };
  int mDoneAfter;
};

/// Sleeps briefly and returns Continue forever; the Runner ends it when stop is requested between
/// iterations.
class ForeverRoutine final: public Routine
{
public:
  ForeverRoutine(): Routine("ForeverRoutine") {}

  State step() final
  {
    std::this_thread::sleep_for(1ms);
    return State::Continue;
  }
};

/// Throws on the first iteration to exercise the Runner's failure path.
class ThrowingRoutine final: public Routine
{
public:
  ThrowingRoutine(): Routine("ThrowingRoutine") {}

  State step() final
  {
    throw std::runtime_error("boom");
  }
};

} // namespace

TEST(ThreadedRunnerTest, runsUntilDone)
{
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<CountingRoutine>(3);

  runner->launch(routine);
  runner->waitUntilStopped();

  EXPECT_EQ(routine->iterations(), 3);
}

TEST(ThreadedRunnerTest, requestStopEndsTheLoop)
{
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<ForeverRoutine>();

  runner->launch(routine);
  runner->requestStop();

  // waitUntilStopped only returns once the loop has observed the stop and ended; a hang here would
  // fail the test by timing out.
  runner->waitUntilStopped();
}

TEST(ThreadedRunnerTest, exceptionEndsTheLoop)
{
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<ThrowingRoutine>();

  runner->launch(routine);

  // A throwing step is caught by the runner, which ends the loop rather than propagating; the join
  // below returns instead of hanging.
  runner->waitUntilStopped();
}

TEST(ThreadedRunnerTest, drivesSeveralRoutines)
{
  const auto runnerA = std::make_shared<ThreadedRunner>();
  const auto runnerB = std::make_shared<ThreadedRunner>();
  const auto routineA = std::make_shared<CountingRoutine>(2);
  const auto routineB = std::make_shared<CountingRoutine>(5);

  runnerA->launch(routineA);
  runnerB->launch(routineB);

  runnerA->waitUntilStopped();
  runnerB->waitUntilStopped();

  EXPECT_EQ(routineA->iterations(), 2);
  EXPECT_EQ(routineB->iterations(), 5);
}

} // namespace nioc::concurrent
