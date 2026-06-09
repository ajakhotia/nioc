////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026.
//  Project  : niocRosBridge
//  Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <utility>

namespace nioc::terminus
{

Driver::Driver(Port& port, std::string name):
  Routine(std::move(name)),
  mPort(port),
  mShutdownToken(port.shutdownToken())
{
}

const std::stop_token& Driver::shutdownToken() const noexcept
{
  return mShutdownToken;
}

Driver::State Driver::step() noexcept
{
  try
  {
    return run();
  }
  catch(const std::exception& exception)
  {
    logger::error("[{}] caught an exception: {}.", name(), exception.what());
  }
  catch(...)
  {
    logger::error("[{}] caught unknown exception.", name());
  }

  return State::Done;
}

} // namespace nioc::terminus
