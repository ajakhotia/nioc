////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <nioc/concurrent/routine.hpp>
#include <string>
#include <utility>

namespace nioc::concurrent
{

Routine::Routine(std::string name, std::function<void()> notifier):
  mName(std::move(name)),
  mNotifier(std::move(notifier))
{
}

void Routine::attachNotifier(std::function<void()> notifier)
{
  mNotifier = std::move(notifier);
}

void Routine::wakeRunner() const
{
  if(mNotifier)
  {
    mNotifier();
  }
}

} // namespace nioc::concurrent
