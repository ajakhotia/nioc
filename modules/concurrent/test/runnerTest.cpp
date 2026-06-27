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

class GatedRoutine final: public Routine
{
public:
  GatedRoutine(): Routine("GatedRoutine") {}

  void release()
  {
    mReleased.store(true);
    triggerRunner();
  }

private:
  std::atomic<bool> mReleased{false};

  State step() noexcept final
  {
    return mReleased.load() ? State::Done : State::Waiting;
  }
};

class FlickerRoutine final: public Routine
{
public:
  explicit FlickerRoutine(const int steps): Routine("FlickerRoutine"), mRemaining{steps} {}

  void poke()
  {
    triggerRunner();
  }

private:
  std::atomic<int> mRemaining;

  State step() noexcept final
  {
    const auto left = mRemaining.fetch_sub(1) - 1;
    if(left <= 0)
    {
      return State::Done;
    }
    return (left % 2 == 0) ? State::Continue : State::Waiting;
  }
};

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
    return mIndex < mScript.size() ? mScript.at(mIndex++) : State::Done;
  }
};

} // namespace

TEST(ThreadedRunnerTest, runsUntilDone)
{
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<CountingRoutine>(3);

  runner->launch(routine);

  // The loop ends on its own once the routine reports Done; poll its recorded State to observe it.
  while(routine->state() != Routine::State::Done)
  {
    std::this_thread::sleep_for(1ms);
  }

  EXPECT_EQ(routine->iterations(), 3);
}

TEST(ThreadedRunnerTest, destructionStopsTheLoop)
{
  auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<ForeverRoutine>();

  runner->launch(routine);

  // Destroying the Runner stops and joins its thread; a hang here would fail the test by timing
  // out.
  runner.reset();
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

TEST(ThreadedRunnerTest, waitingRoutineResumesOnTrigger)
{
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<GatedRoutine>();

  runner->launch(routine);

  // The routine parks the runner by reporting Waiting; without a trigger it would never run again.
  while(routine->state() != Routine::State::Waiting)
  {
    std::this_thread::sleep_for(1ms);
  }

  routine->release();

  // Reaching Done proves the trigger woke the parked loop; a missed wakeup would hang here.
  while(routine->state() != Routine::State::Done)
  {
    std::this_thread::sleep_for(1ms);
  }
}

TEST(ThreadedRunnerTest, wakeHandshakeNeverLosesAWakeup)
{
  // The trigger latches mReady under the runner's mutex, so a trigger firing in the window between
  // a Waiting return and the park must still wake the thread. Hammer that window: poke the routine
  // the moment it reports Waiting, thousands of times. A lost wakeup parks the loop forever and
  // trips the deadline below.
  constexpr auto kSteps = 20'000;
  const auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<FlickerRoutine>(kSteps);

  runner->launch(routine);

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
  while(routine->state() != Routine::State::Done)
  {
    ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "lost wakeup: routine never finished";
    routine->poke();
  }
}

TEST(ThreadedRunnerTest, destructionWakesAParkedRoutine)
{
  auto runner = std::make_shared<ThreadedRunner>();
  const auto routine = std::make_shared<GatedRoutine>();

  runner->launch(routine);

  // Park the loop: the routine reports Waiting and is never released.
  while(routine->state() != Routine::State::Waiting)
  {
    std::this_thread::sleep_for(1ms);
  }

  // The jthread's stop request must wake the parked thread; a missed wakeup would hang the join.
  runner.reset();
}

TEST(RoutineTest, triggerWithoutRunnerIsSafe)
{
  // No runner attached: release() fires triggerRunner with no trigger installed, which is a no-op.
  auto routine = GatedRoutine{};
  routine.release();
  EXPECT_EQ(routine.tick(), Routine::State::Done);
}

TEST(ThreadedRunnerTest, drivesSeveralRoutines)
{
  const auto runnerA = std::make_shared<ThreadedRunner>();
  const auto runnerB = std::make_shared<ThreadedRunner>();
  const auto routineA = std::make_shared<CountingRoutine>(2);
  const auto routineB = std::make_shared<CountingRoutine>(5);

  runnerA->launch(routineA);
  runnerB->launch(routineB);

  while(routineA->state() != Routine::State::Done || routineB->state() != Routine::State::Done)
  {
    std::this_thread::sleep_for(1ms);
  }

  EXPECT_EQ(routineA->iterations(), 2);
  EXPECT_EQ(routineB->iterations(), 5);
}

} // namespace nioc::concurrent
