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

void Routine::attachTrigger(std::function<void()> trigger)
{
  mTrigger = std::move(trigger);
}

Routine::Routine(std::string name, std::function<void()> trigger):
  mName(std::move(name)),
  mTrigger(std::move(trigger))
{
}

void Routine::triggerRunner() const
{
  if(mTrigger)
  {
    mTrigger();
  }
}

} // namespace nioc::concurrent
