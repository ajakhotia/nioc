////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

namespace nioc::terminus
{

/// @brief A unit of work executed one iteration at a time by a @ref Runner.
///
/// The iteration loop lives in the Runner, not here: @ref step performs a single iteration and
/// reports what should happen next. This keeps every Routine free of stop-token plumbing and
/// exception bookkeeping, which the Runner owns in one place.
///
/// A Routine signals its outcome through the returned @ref State:
/// - @ref State::Continue to be scheduled again immediately,
/// - @ref State::Waiting when it has no work right now and should be rescheduled later,
/// - @ref State::Done to finish naturally (end of input, deadline reached);
/// or it throws to fail, which the Runner catches, logs, and treats as a stop.
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

  Routine() = default;

  Routine(const Routine&) = delete;

  Routine(Routine&&) noexcept = delete;

  virtual ~Routine() = default;

  Routine& operator=(const Routine&) = delete;

  Routine& operator=(Routine&&) noexcept = delete;

  /// @brief Performs one iteration of work.
  ///
  /// @return @ref State::Continue to run again immediately, @ref State::Waiting to be rescheduled
  /// later, or @ref State::Done when finished.
  ///
  /// @throws std::exception  Actual throw depends on the implementation and may vary.
  [[nodiscard]] virtual State step() = 0;

  /// @brief Returns a human-readable name.
  [[nodiscard]] virtual std::string_view name() const = 0;

  void attachNotifier(std::function<void()> notifier)
  {
    mNotifyReady = std::move(notifier);
  }

protected:
  void notifyReady()
  {
    if(mNotifyReady)
    {
      mNotifyReady();
    }
  }

private:
  std::function<void()> mNotifyReady;
};

} // namespace nioc::terminus
