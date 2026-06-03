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

/// @brief Drives one @ref Routine's iteration loop: repeatedly call @ref Routine::step until done.
///
/// A Runner owns the loop a @ref Routine deliberately lacks. After @ref launch, it calls the
/// routine's @ref Routine::step over and over: @ref State::Continue runs again immediately, @ref
/// State::Waiting parks until the routine signals it has some work, and @ref State::Done (or a
/// thrown exception) ends the loop. The waiting/wake handshake lets an idle routine sleep instead
/// of spinning; @ref ThreadedRunner is the thread-backed implementation.
///
/// A Runner is held through a `shared_ptr` (it derives from `enable_shared_from_this` so it can
/// hand the routine a notifier that outlives the call). Each Runner drives a single routine.
class Runner: public std::enable_shared_from_this<Runner>
{
public:
  Runner() = default;

  Runner(const Runner&) = delete;

  Runner(Runner&&) noexcept = delete;

  virtual ~Runner() = default;

  Runner& operator=(const Runner&) = delete;

  Runner& operator=(Runner&&) noexcept = delete;

  /// @brief Starts driving @p routine's step loop.
  ///
  /// The Runner holds the routine weakly and stops on its own once the routine expires. Call once
  /// per Runner.
  ///
  /// @param routine Routine to drive.
  virtual void launch(std::weak_ptr<Routine> routine) = 0;

  /// @brief Blocks the caller until the driven routine's loop has ended.
  ///
  /// Returns once the routine reported @ref State::Done, threw, expired, or @ref requestStop took
  /// effect.
  virtual void waitUntilStopped() = 0;

  /// @brief Asks the loop to stop after the current iteration; returns without waiting.
  ///
  /// Pair with @ref waitUntilStopped to block until the loop has actually ended.
  virtual void requestStop() noexcept = 0;

protected:
  /// @brief Builds the notifier handed to the routine so it can wake this Runner from @ref
  /// State::Waiting.
  ///
  /// The notifier holds the Runner weakly, so it is safe to invoke after the Runner is gone.
  [[nodiscard]] std::function<void()> makeNotifier();

  /// @brief Wakes a Runner parked because its routine returned @ref State::Waiting.
  virtual void wake() = 0;
};

} // namespace nioc::concurrent
