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

/// @brief Abstract base for an object that drives one Routine on an execution context, such as a
/// thread, supplied by the concrete subclass.
///
/// A Runner repeatedly ticks its Routine, parks it while the Routine reports State::Waiting, and
/// resumes it when the Routine's wake trigger fires. This base provides only the trigger closure
/// via makeTrigger(); subclasses supply the execution context and the wake mechanism. To use,
/// derive a concrete Runner, create it with std::make_shared, then call launch() with a Routine.
///
/// Non-copyable and non-movable. Must be owned through a std::shared_ptr: makeTrigger() captures
/// weak_from_this(), so a Runner not managed by a shared_ptr can never be woken.
///
/// @see Routine, makeTrigger
class Runner: public std::enable_shared_from_this<Runner>
{
public:
  Runner() = default;

  Runner(const Runner&) = delete;

  Runner(Runner&&) noexcept = delete;

  virtual ~Runner() = default;

  Runner& operator=(const Runner&) = delete;

  Runner& operator=(Runner&&) noexcept = delete;

  /// @brief Start driving @p routine on this Runner's execution context.
  ///
  /// The Routine is held weakly: the caller must keep a shared_ptr to it alive for as long as it
  /// should run; once the Routine expires, the Runner stops. Implementations attach the closure
  /// from makeTrigger() to the Routine before the first tick. Call at most once per Runner.
  ///
  /// @param routine The Routine to drive. If it has already expired at the call, the Runner has
  /// nothing to drive and stops.
  virtual void launch(std::weak_ptr<Routine> routine) = 0;

protected:
  /// @brief Build the wake callback that a Routine invokes to ask this Runner to resume ticking.
  ///
  /// The returned closure calls wake() on this Runner. It captures only a weak reference, so it is
  /// safe to outlive the Runner: once the Runner is destroyed, invoking it is a no-op. A subclass
  /// hands the result to its Routine via Routine::attachTrigger, typically during launch().
  ///
  /// @return A closure that invokes wake() while the Runner is alive, else does nothing.
  ///
  /// @see wake, Routine::attachTrigger
  [[nodiscard]] std::function<void()> makeTrigger();

  /// @brief Resume this Runner's parked execution context so it ticks the Routine again.
  ///
  /// Called from the trigger built by makeTrigger(), typically from a thread other than the
  /// execution context, so the override must be thread-safe.
  ///
  /// @see makeTrigger
  virtual void wake() = 0;
};

} // namespace nioc::concurrent
