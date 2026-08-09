////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <nioc/common/exception.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/component.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace nioc::terminus
{
namespace
{

concurrent::BufferMode toConcurrentBufferMode(const BufferMode bufferMode)
{
  switch(bufferMode)
  {
    case BufferMode::OVERWRITING:
      return concurrent::BufferMode::Overwriting;
    case BufferMode::UNBOUNDED:
      return concurrent::BufferMode::Unbounded;
  }
  common::throwException<std::invalid_argument>(
      "{} names no buffer mode",
      static_cast<std::uint16_t>(bufferMode));
}

} // namespace

Component::Component(
    std::string name,
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Routine(std::move(name)),
  mPort(port),
  mInbox([this] { triggerRunner(); }, bufferMode, inboxCapacity)
{
}

Component::Component(std::string name, Port& port, const ComponentConfig::Reader config):
  Component{
      std::move(name),
      port,
      config.getInboxCapacity(),
      toConcurrentBufferMode(config.getBufferMode())}
{
}

Component::State Component::step() noexcept
{
  try
  {
    auto value = mInbox.tryPop();
    if(not value)
    {
      return State::Waiting;
    }

    // Dispatch hands the consignment to the subscribed callback, which returns the next State. The
    // consignment is destroyed when the callback returns, decrementing the port's in-flight counter
    // to report the delivery.
    return std::invoke(*value->first, std::move(value->second));
  }
  catch(const std::exception& exception)
  {
    logger::error("[{}] {}", name(), exception.what());
  }
  catch(...)
  {
    logger::error("[{}] unhandled exception", name());
  }

  return State::Done;
}


} // namespace nioc::terminus
