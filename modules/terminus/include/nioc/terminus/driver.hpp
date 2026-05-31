////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "port.hpp"
#include "routine.hpp"

namespace nioc::terminus
{

class Driver: public Routine
{
public:
  Driver(const Driver&) = delete;
  Driver(Driver&&) noexcept = delete;
  Driver& operator=(const Driver&) = delete;
  Driver& operator=(Driver&&) noexcept = delete;
  ~Driver() noexcept override = default;

protected:
  explicit Driver(Port& port) noexcept: mPort(port) {}

  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  Port& mPort;
};

} // namespace nioc::terminus
