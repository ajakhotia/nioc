////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <memory>
#include <nioc/concurrent/runner.hpp>

namespace nioc::concurrent
{

std::function<void()> Runner::makeTrigger()
{
  return [self = weak_from_this()]
  {
    if(const auto runner = self.lock())
    {
      runner->wake();
    }
  };
}

} // namespace nioc::concurrent
