////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "routine.hpp"
#include <functional>
#include <memory>

namespace nioc::concurrent
{

/// @brief Calls one @ref Routine's @ref Routine::step in a loop until it is done.
///
/// After @ref launch, the Runner calls @ref Routine::step again and again. The return value picks
/// what happens next: @ref State::Continue steps again right away, @ref State::Waiting parks until
/// the routine triggers a wake, and @ref State::Done ends the loop. @ref Routine::step never
/// throws; a failing routine returns Done. @ref ThreadedRunner runs the loop on a thread.
///
/// Hold a Runner through a `shared_ptr`. Each Runner drives one routine.
class Runner: public std::enable_shared_from_this<Runner>
{
public:
  Runner() = default;

  Runner(const Runner&) = delete;

  Runner(Runner&&) noexcept = delete;

  virtual ~Runner() = default;

  Runner& operator=(const Runner&) = delete;

  Runner& operator=(Runner&&) noexcept = delete;

  /// @brief Starts the step loop for @p routine.
  ///
  /// The Runner holds the routine weakly. Once the routine expires, the loop stops at the next step
  /// or wake (not the instant it expires). Call once per Runner.
  ///
  /// @param routine Routine to drive.
  virtual void launch(std::weak_ptr<Routine> routine) = 0;

protected:
  /// @brief Builds the trigger the routine calls to wake this Runner from @ref State::Waiting.
  ///
  /// The trigger holds the Runner weakly. Safe to call after the Runner is gone.
  [[nodiscard]] std::function<void()> makeTrigger();

  /// @brief Wakes a Runner parked on @ref State::Waiting.
  virtual void wake() = 0;
};

} // namespace nioc::concurrent
