////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "routine.hpp"
#include <functional>
#include <memory>

namespace nioc::terminus
{

class Runner: public std::enable_shared_from_this<Runner>
{
public:
  Runner() = default;

  Runner(const Runner&) = delete;

  Runner(Runner&&) noexcept = delete;

  virtual ~Runner() = default;

  Runner& operator=(const Runner&) = delete;

  Runner& operator=(Runner&&) noexcept = delete;

  virtual void launch(std::weak_ptr<Routine> routine) = 0;

  virtual void waitUntilStopped() = 0;

  virtual void requestStop() noexcept = 0;

protected:
  [[nodiscard]] std::function<void()> makeNotifier();

  virtual void wake() = 0;
};

} // namespace nioc::terminus
