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

/// @brief A unit of work executed one iteration at a time by a @ref Runner.
///
/// The iteration loop lives in the Runner, not here: the @ref step performs a single iteration and
/// reports what should happen next. This keeps every Routine free of stop-token plumbing and
/// exception bookkeeping, which the Runner owns in one place.
///
/// A Routine signals its outcome through the returned @ref State:
/// - @ref State::Continue to be scheduled again immediately,
/// - @ref State::Waiting when it has no work right now and should be rescheduled later,
/// - @ref State::Done to finish naturally (end of input, deadline reached) or to fail;
/// @ref step never throws — an implementation that can fail catches its own exceptions and reports
/// @ref State::Done.
class Routine
{
public:
  /// @brief Outcome of a single @ref step iteration.
  enum class State : std::uint8_t
  {
    /// @brief Has more work now; schedule another iteration immediately.
    Continue,

    /// @brief Has no work right now; reschedule later rather than spinning.
    Waiting,

    /// @brief Finished naturally; stop scheduling it.
    Done
  };

  /// @brief Names the routine and, optionally, installs its ready-trigger.
  ///
  /// @param name Human-readable identity for this routine; see @ref name.
  ///
  /// @param trigger A callback the routine fires (through @ref triggerRunner) to wake its driving
  /// Runner when work arrives. Defaults to none; the Runner may instead install one later via @ref
  /// attachTrigger.
  explicit Routine(std::string name, std::function<void()> trigger = nullptr);

  Routine(const Routine&) = delete;

  Routine(Routine&&) noexcept = delete;

  virtual ~Routine() = default;

  Routine& operator=(const Routine&) = delete;

  Routine& operator=(Routine&&) noexcept = delete;

  /// @brief Advances the routine by one iteration and records its outcome.
  ///
  /// Called by the driving @ref Runner once per loop iteration. Runs @ref step, stores the returned
  /// @ref State so it can be observed from another thread via @ref state, and returns it. Never
  /// throws: a @ref step that fails reports @ref State::Done rather than escaping.
  ///
  /// @return The @ref State @ref step reported for this iteration.
  [[nodiscard]] State tick() noexcept
  {
    const auto state = step();
    mState.store(state, std::memory_order_relaxed);
    return state;
  }

  /// @brief Returns this routine's name: its human-readable identity.
  [[nodiscard]] const std::string& name() const noexcept
  {
    return mName;
  }

  /// @brief Returns the @ref State the most recent @ref tick recorded.
  ///
  /// Safe to call from a thread other than the one driving @ref tick: it reports the outcome of the
  /// last completed iteration (@ref State::Continue before the first one).
  [[nodiscard]] State state() const noexcept
  {
    return mState.load(std::memory_order_relaxed);
  }

  /// @brief Installs the callback the routine uses to announce it has some work again.
  ///
  /// Called by the @ref Runner that drives this routine. A routine that returned @ref
  /// State::Waiting fires the trigger (through @ref triggerRunner) once new work arrives, so the
  /// Runner can resume it instead of polling.
  ///
  /// @param trigger Callback that wakes the driving Runner.
  void attachTrigger(std::function<void()> trigger);

protected:
  /// @brief Signals the driving @ref Runner that work is available if a trigger is attached.
  ///
  /// A subclass calls this when it transitions from idle to having work, to wake a Runner parked
  /// after a @ref State::Waiting return. No-op while no trigger is set (none passed at
  /// construction and @ref attachTrigger not yet called).
  void triggerRunner() const;

private:
  const std::string mName;
  std::function<void()> mTrigger{nullptr};
  std::atomic<State> mState{State::Continue};

  /// @brief Performs one iteration of work; implemented by a subclass and run from @ref tick.
  ///
  /// Must not throw: an implementation that can fail catches its own exceptions and reports @ref
  /// State::Done so the Runner winds the routine down cleanly.
  ///
  /// @return @ref State::Continue to run again immediately, @ref State::Waiting to be rescheduled
  /// later, or @ref State::Done when finished.
  [[nodiscard]] virtual State step() noexcept = 0;
};

} // namespace nioc::concurrent
