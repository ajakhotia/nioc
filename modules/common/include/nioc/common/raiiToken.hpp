////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace nioc::common
{

/// @brief A scope guard that runs a setup action at construction and a matching teardown action
/// when it is destroyed, giving any acquire/release pair classic RAII semantics.
///
/// Construct the token to run the entry action once, immediately. The stored exit action then runs
/// once, automatically, when the token is destroyed (or replaced via move-assignment). Keep the
/// token alive while the effect must hold; let it leave scope to release.
///
/// Example:
///
///     // Lock now, unlock when `guard` leaves scope.
///     RaiiToken guard{[&] { mutex.lock(); }, [&]() noexcept { mutex.unlock(); }};
///
/// Moving transfers the exit action: the moved-from token is disengaged and runs nothing on
/// destruction. Copying is disabled. Marked `[[nodiscard]]` because ignoring the returned token
/// destroys it at once, firing the exit action before the guarded effect is used.
///
/// @tparam ExitAction_ The stored teardown callable. Must be nothrow-invocable with no arguments
/// and nothrow-move-constructible, because it is invoked during `noexcept` destruction and
/// move-assignment, and relocated during `noexcept` move construction and move-assignment. Deduce
/// it from the constructor; do not name it explicitly.
template<std::invocable ExitAction_>
  requires std::is_nothrow_invocable_v<ExitAction_> &&
           std::is_nothrow_move_constructible_v<ExitAction_>
class [[nodiscard]] RaiiToken
{
public:
  using ExitAction = ExitAction_;

  /// @brief Run @p entryAction immediately and store @p exitAction to run when the token is
  /// destroyed.
  ///
  /// If @p entryAction throws, construction fails and the exit action never runs. The exit action
  /// is always invoked with no arguments; @p args reach the entry action only.
  ///
  /// @tparam EntryAction Type of the setup callable, deduced from @p entryAction.
  ///
  /// @tparam Args Types of the trailing arguments, deduced from @p args.
  ///
  /// @param entryAction Setup callable, invoked once with @p args.
  ///
  /// @param exitAction Teardown callable, stored by value and invoked later with no arguments.
  ///
  /// @param args Forwarded to @p entryAction only.
  template<typename EntryAction, typename... Args>
    requires std::invocable<EntryAction, Args...>
  explicit RaiiToken(EntryAction&& entryAction, ExitAction exitAction, Args&&... args):
    mExitAction{std::move(exitAction)},
    mEngaged{true}
  {
    std::invoke(std::forward<EntryAction>(entryAction), std::forward<Args>(args)...);
  }

  RaiiToken(const RaiiToken&) = delete;

  RaiiToken& operator=(const RaiiToken&) = delete;

  /// @brief Adopt @p other's exit action, leaving @p other disengaged so only this token runs it.
  RaiiToken(RaiiToken&& other) noexcept:
    mExitAction{std::move(other.mExitAction)},
    mEngaged{std::exchange(other.mEngaged, false)}
  {
  }

  /// @brief Run this token's pending exit action, then adopt @p other's, leaving @p other
  /// disengaged.
  ///
  /// Self-assignment is a no-op and does not fire the exit action.
  RaiiToken& operator=(RaiiToken&& other) noexcept
  {
    if(this != &other)
    {
      if(mEngaged)
      {
        std::invoke(mExitAction);
      }

      // A closure type is not move-assignable, so relocate the action by destroying the current one
      // and move-constructing the incoming one in place.
      std::destroy_at(std::addressof(mExitAction));
      std::construct_at(std::addressof(mExitAction), std::move(other.mExitAction));
      mEngaged = std::exchange(other.mEngaged, false);
    }

    return *this;
  }

  /// @brief Run the exit action, unless the token has been moved from.
  ~RaiiToken() noexcept
  {
    if(mEngaged)
    {
      std::invoke(mExitAction);
    }
  }

private:
  /// The teardown callable to invoke once when this token is destroyed or move-assigned over.
  [[no_unique_address]] ExitAction mExitAction;

  /// Whether this token still owns a pending exit action. False after the token is moved from, so
  /// destruction and move-assignment skip the exit action.
  bool mEngaged{false};
};

/// @brief Deduce `RaiiToken` from its constructor arguments, fixing the template parameter to the
/// exit action's type so the entry action and args stay unconstrained at deduction.
template<typename EntryAction, typename ExitAction, typename... Args>
RaiiToken(EntryAction&&, ExitAction, Args&&...) -> RaiiToken<ExitAction>;

} // namespace nioc::common
