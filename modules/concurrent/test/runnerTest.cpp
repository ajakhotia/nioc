////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/concurrent/routine.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <thread>
#include <utility>
#include <vector>

namespace nioc::concurrent
{
namespace
{

using namespace std::chrono_literals;

/// Returns Done after a fixed number of iterations, counting how many times it ran.
class CountingRoutine final: public Routine
{
public:
  explicit CountingRoutine(const int doneAfter): Routine("CountingRoutine"), mDoneAfter{doneAfter}
  {
  }

  [[nodiscard]] int iterations() const
  {
    return mIterations.load();
  }

private:
  std::atomic<int> mIterations{0};
  int mDoneAfter;

  State step() noexcept final
  {
    if(++mIterations >= mDoneAfter)
    {
      return State::Done;
    }
    return State::Continue;
  }
};

/// Sleeps briefly and returns Continue forever; the Runner ends it when stop is requested between
/// iterations.
class ForeverRoutine final: public Routine
{
public:
  ForeverRoutine(): Routine("ForeverRoutine") {}

private:
  State step() noexcept final
  {
    std::this_thread::sleep_for(1ms);
    return State::Continue;
  }
};

/// Reports a scripted sequence of States, one per step, to exercise state() recording.
class ScriptedRoutine final: public Routine
{
public:
  explicit ScriptedRoutine(std::vector<State> script):
    Routine("ScriptedRoutine"),
    mScript(std::move(script))
  {
  }

private:
  std::vector<State> mScript;
  std::size_t mIndex{0};

  State step() noexcept final
  {
    return mIndex < mScript.size() ? mScript[mIndex++] : State::Done;
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

  // Once the loop ends on Done, the routine reports that final State to any later observer.
  EXPECT_EQ(routine->state(), Routine::State::Done);
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

TEST(RoutineTest, tickRecordsLastReportedState)
{
  auto routine = ScriptedRoutine(
      {Routine::State::Waiting, Routine::State::Continue, Routine::State::Done});

  // Before the first tick the routine reports Continue: runnable, not yet started.
  EXPECT_EQ(routine.state(), Routine::State::Continue);

  EXPECT_EQ(routine.tick(), Routine::State::Waiting);
  EXPECT_EQ(routine.state(), Routine::State::Waiting);

  EXPECT_EQ(routine.tick(), Routine::State::Continue);
  EXPECT_EQ(routine.state(), Routine::State::Continue);

  EXPECT_EQ(routine.tick(), Routine::State::Done);
  EXPECT_EQ(routine.state(), Routine::State::Done);
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
