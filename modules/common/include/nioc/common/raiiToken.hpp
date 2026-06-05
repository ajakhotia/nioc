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

template<std::invocable ExitAction_>
  requires std::is_nothrow_invocable_v<ExitAction_> &&
           std::is_nothrow_move_constructible_v<ExitAction_>
class [[nodiscard]] RaiiToken
{
public:
  using ExitAction = ExitAction_;

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

  RaiiToken(RaiiToken&& other) noexcept:
    mExitAction{std::move(other.mExitAction)},
    mEngaged{std::exchange(other.mEngaged, false)}
  {
  }

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

  ~RaiiToken() noexcept
  {
    if(mEngaged)
    {
      std::invoke(mExitAction);
    }
  }

private:
  [[no_unique_address]] ExitAction mExitAction;
  bool mEngaged;
};

template<typename EntryAction, typename ExitAction, typename... Args>
RaiiToken(EntryAction&&, ExitAction, Args&&...) -> RaiiToken<ExitAction>;

} // namespace nioc::common
