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

/// @brief A unit of work run one iteration at a time by a @ref Runner.
///
/// A @ref Runner drives the loop. Each turn it calls @ref step, which does one iteration and
/// returns a @ref State telling the Runner what to do next:
/// - @ref State::Continue: run again right away,
/// - @ref State::Waiting: no work now, run again later,
/// - @ref State::Done: finished or failed, stop scheduling.
///
/// @ref step never throws: if it can fail, it catches the exception and returns @ref State::Done.
class Routine
{
public:
  /// @brief Result of one @ref step iteration.
  enum class State : std::uint8_t
  {
    /// @brief More work now; run again immediately.
    Continue,

    /// @brief No work now; run again later instead of spinning.
    Waiting,

    /// @brief Finished or failed; stop scheduling it.
    Done
  };

  Routine(const Routine&) = delete;

  Routine(Routine&&) noexcept = delete;

  virtual ~Routine() = default;

  Routine& operator=(const Routine&) = delete;

  Routine& operator=(Routine&&) noexcept = delete;

  /// @brief Runs one iteration and records its result.
  ///
  /// Runs @ref step and stores the returned @ref State so @ref state can read it from another
  /// thread. Never throws. Called by the driving @ref Runner once per loop turn.
  ///
  /// @return The @ref State that @ref step returned.
  [[nodiscard]] State tick() noexcept
  {
    const auto state = step();
    mState.store(state, std::memory_order_relaxed);
    return state;
  }

  /// @brief Returns this routine's human-readable name.
  [[nodiscard]] const std::string& name() const noexcept
  {
    return mName;
  }

  /// @brief Returns the @ref State from the last @ref tick.
  [[nodiscard]] State state() const noexcept
  {
    return mState.load(std::memory_order_relaxed);
  }

  /// @brief Sets the callback that wakes the Runner when work arrives.
  ///
  /// After returning @ref State::Waiting, the routine fires this callback (via @ref triggerRunner)
  /// when new work arrives, so the Runner resumes it instead of polling.
  ///
  /// @param trigger Callback that wakes the driving @ref Runner.
  void attachTrigger(std::function<void()> trigger);

protected:
  /// @brief Sets the name and, optionally, the wake-up callback.
  ///
  /// @param name Human-readable name; see @ref name.
  ///
  /// @param trigger Callback the routine fires (via @ref triggerRunner) to wake its @ref Runner
  /// when work arrives. Defaults to none; the Runner can set one later via @ref attachTrigger.
  explicit Routine(std::string name, std::function<void()> trigger = nullptr);

  /// @brief Wakes the driving @ref Runner if a trigger is set.
  ///
  /// A subclass calls this when it goes from idle to having work, to wake a Runner parked after a
  /// @ref State::Waiting return. Does nothing while no trigger is set (none passed at construction
  /// and @ref attachTrigger not yet called).
  void triggerRunner() const;

private:
  const std::string mName;
  std::function<void()> mTrigger{nullptr};
  std::atomic<State> mState{State::Continue};

  /// @brief Does one iteration of work; a subclass implements it, @ref tick runs it.
  ///
  /// Must not throw: if it can fail, catch the exception and return @ref State::Done so the Runner
  /// shuts the routine down cleanly.
  ///
  /// @return @ref State::Continue to run again now, @ref State::Waiting to run again later, or
  /// @ref State::Done when finished or failed.
  [[nodiscard]] virtual State step() noexcept = 0;
};

} // namespace nioc::concurrent
