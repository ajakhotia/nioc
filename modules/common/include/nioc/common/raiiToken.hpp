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

/// RAII guard. Runs an entry action now and an exit action at scope exit.
///
/// The exit action runs once when the token is destroyed. A moved-from token runs nothing.
///
/// @tparam ExitAction_ No-argument callable run at scope exit. Must be no-throw to call and
/// no-throw move-constructible.
template<std::invocable ExitAction_>
  requires std::is_nothrow_invocable_v<ExitAction_> &&
           std::is_nothrow_move_constructible_v<ExitAction_>
class [[nodiscard]] RaiiToken
{
public:
  /// The exit action callable type.
  using ExitAction = ExitAction_;

  /// Runs the entry action now. Stores the exit action to run at scope exit.
  ///
  /// @param entryAction Callable run now with @p args.
  /// @param exitAction  Callable run when the token is destroyed.
  /// @param args        Arguments forwarded to @p entryAction.
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

  /// Takes over the pending exit action from @p other, which is left holding none.
  RaiiToken(RaiiToken&& other) noexcept:
    mExitAction{std::move(other.mExitAction)},
    mEngaged{std::exchange(other.mEngaged, false)}
  {
  }

  /// Runs this token's exit action, then takes over the one from @p other.
  ///
  /// @param other Source token, left holding no exit action.
  /// @return Reference to this token.
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

  /// Runs the exit action, unless the token has been moved from.
  ~RaiiToken() noexcept
  {
    if(mEngaged)
    {
      std::invoke(mExitAction);
    }
  }

private:
  [[no_unique_address]] ExitAction mExitAction;
  bool mEngaged{false};
};

/// Deduces the token's exit action type from the constructor arguments.
template<typename EntryAction, typename ExitAction, typename... Args>
RaiiToken(EntryAction&&, ExitAction, Args&&...) -> RaiiToken<ExitAction>;

} // namespace nioc::common
