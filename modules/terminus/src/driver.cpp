////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026.
//  Project  : niocRosBridge
//  Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/blob.h>
#include <exception>
#include <nioc/common/exception.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/driver.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace nioc::terminus
{
namespace
{

std::string requiredName(const capnp::Text::Reader name)
{
  if(name.size() == 0)
  {
    common::throwException<std::invalid_argument>("Driver config must provide a non-empty name");
  }
  return {name.begin(), name.end()};
}

} // namespace

Driver::Driver(Port& port, std::string name):
  Routine(std::move(name)),
  mPort(port),
  mShutdownToken(port.shutdownToken())
{
}

Driver::Driver(Port& port, const DriverConfig::Reader config):
  Driver{port, requiredName(config.getName())}
{
}

const std::stop_token& Driver::shutdownToken() const noexcept
{
  return mShutdownToken;
}

Port& Driver::port() noexcept
{
  return mPort;
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
