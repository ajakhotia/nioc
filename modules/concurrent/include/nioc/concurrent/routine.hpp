////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace nioc::concurrent
{

/// @brief Abstract base for a unit of cooperative, non-blocking work that a Runner drives one
/// step at a time.
///
/// Derive from `Routine`, give it a name, and override the private `step()` to do one small slice
/// of work per call and report what to do next. A Runner owns the execution context (e.g. a
/// thread): it repeatedly calls `tick()` and acts on the returned `State`, parking when the
/// routine has nothing to do and resuming when the routine fires its trigger. The routine must
/// never block; it yields control back to the Runner by returning from `step()`.
///
/// Example:
///
///     class Counter : public Routine
///     {
///     public:
///       Counter() : Routine("counter") {}
///     private:
///       State step() noexcept override
///       {
///         return (++mCount < 10) ? State::Continue : State::Done;
///       }
///       int mCount{0};
///     };
///
/// Non-copyable and non-movable. A `Routine` is meant to be ticked serially from a single
/// context; `state()` may be read concurrently from other threads.
///
/// @see step, tick, attachTrigger, triggerRunner
class Routine
{
public:
  /// @brief Outcome of one step, telling the driving Runner how to proceed.
  enum class State : std::uint8_t
  {
    /// More work is ready now; tick again immediately.
    Continue,

    /// No work available; park the runner until the trigger fires, then tick again.
    Waiting,

    /// Work is complete or unrecoverable; stop driving this routine.
    Done
  };

  Routine(const Routine&) = delete;

  Routine(Routine&&) noexcept = delete;

  virtual ~Routine() = default;

  Routine& operator=(const Routine&) = delete;

  Routine& operator=(Routine&&) noexcept = delete;

  /// @brief Run one step and publish its result.
  ///
  /// Calls the overridden `step()` once, stores the result so `state()` can read it, and returns
  /// it. Meant to be called by the driving Runner, serially from a single context.
  ///
  /// @return The `State` reported by `step()`.
  [[nodiscard]] State tick() noexcept
  {
    const auto state = step();
    mState.store(state, std::memory_order_relaxed);
    return state;
  }

  /// @brief Identifying label, fixed for the routine's lifetime.
  [[nodiscard]] const std::string& name() const noexcept
  {
    return mName;
  }

  /// @brief The `State` recorded by the most recent `tick()`, or `State::Continue` before the
  /// first tick.
  ///
  /// Safe to read from any thread; loaded with relaxed atomics.
  [[nodiscard]] State state() const noexcept
  {
    return mState.load(std::memory_order_relaxed);
  }

  /// @brief Install the callback used to wake the driving Runner, replacing any previous trigger.
  ///
  /// Typically called by the Runner during launch.
  ///
  /// @param trigger Invoked by `triggerRunner()` to resume ticking. May be null.
  ///
  /// @warning Not synchronized against concurrent `tick()` or `triggerRunner()`; attach before the
  /// routine is driven.
  void attachTrigger(std::function<void()> trigger);

protected:
  /// @brief Construct with an identifying name and, optionally, a wake trigger.
  ///
  /// @param name Identifying label; fixed for the routine's lifetime.
  ///
  /// @param trigger Callback that wakes the driving Runner. May be null and supplied later via
  /// `attachTrigger()`.
  explicit Routine(std::string name, std::function<void()> trigger = nullptr);

  /// @brief Invoke the attached trigger to ask the driving Runner to resume ticking.
  ///
  /// Call this from inside the routine when new work arrives so a routine that previously returned
  /// `State::Waiting` gets ticked again. No-op when no trigger is attached.
  ///
  /// @note As thread-safe as the attached trigger (Runner-supplied triggers wake safely from any
  /// thread). The trigger is read without synchronization, so attach it before driving the
  /// routine.
  ///
  /// @see attachTrigger
  void triggerRunner() const;

private:
  /// @brief Identifying label, fixed at construction for the routine's lifetime.
  const std::string mName;

  /// @brief Callback that wakes the driving Runner; null until attached.
  std::function<void()> mTrigger{nullptr};

  /// @brief State reported by the most recent tick, published for concurrent reads.
  std::atomic<State> mState{State::Continue};

  /// @brief Perform one non-blocking slice of work and report what to do next.
  ///
  /// Override to implement the routine. Called serially via `tick()`.
  ///
  /// @return `State::Continue` when more work remains, `State::Waiting` when no work is ready, or
  /// `State::Done` when finished.
  ///
  /// @warning Must not block and must not throw (declared `noexcept`). Handle errors internally
  /// and return `State::Done` to terminate.
  [[nodiscard]] virtual State step() noexcept = 0;
};

} // namespace nioc::concurrent
