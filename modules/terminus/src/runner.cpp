////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <memory>
#include <nioc/terminus/runner.hpp>

namespace nioc::terminus
{

std::function<void()> Runner::makeNotifier()
{
  return [self = weak_from_this()]
  {
    if(const auto runner = self.lock())
    {
      runner->wake();
    }
  };
}

} // namespace nioc::terminus
