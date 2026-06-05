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

/// RAII guard that runs an entry action immediately and a matching exit action at scope exit.
///
/// The constructor invokes the entry action right away. The exit action runs once when the token
/// is destroyed, unless the token has been moved from. A moved-from token holds no pending exit
/// action and runs nothing.
///
/// @tparam ExitAction_ No-throw, no-argument callable run at scope exit.
template<std::invocable ExitAction_>
  requires std::is_nothrow_invocable_v<ExitAction_> &&
           std::is_nothrow_move_constructible_v<ExitAction_>
class [[nodiscard]] RaiiToken
{
public:
  /// The exit action callable type.
  using ExitAction = ExitAction_;

  /// Runs the entry action immediately and stores the exit action for scope exit.
  ///
  /// @param entryAction Callable invoked now with @p args.
  /// @param exitAction  No-throw callable stored and run when the token is destroyed.
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

  /// Runs this token's pending exit action, then takes over the one from @p other.
  ///
  /// @param other Source token, left holding no pending exit action.
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
      // and move-constructing the incoming one in place. For the trivially relocatable closures on
      // the hot path this lowers to a plain move, identical to what an assignment would emit.
      std::destroy_at(std::addressof(mExitAction));
      std::construct_at(std::addressof(mExitAction), std::move(other.mExitAction));
      mEngaged = std::exchange(other.mEngaged, false);
    }

    return *this;
  }

  /// Runs the pending exit action unless the token has been moved from.
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
