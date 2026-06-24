////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/blob.h>
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

/// Maps the config-schema BufferMode onto the concurrent one the inbox understands.
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

/// Returns the instance name a config block provides; an empty one is a config authoring error.
std::string requiredName(const capnp::Text::Reader name)
{
  if(name.size() == 0)
  {
    common::throwException<std::invalid_argument>("Component config must provide a non-empty name");
  }
  return {name.begin(), name.end()};
}

} // namespace

Component::Component(
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode,
    std::string name):
  Routine(std::move(name)),
  mPort(port),
  mInbox([this] { triggerRunner(); }, bufferMode, inboxCapacity)
{
}

Component::Component(Port& port, const ComponentConfig::Reader config):
  Component{
      port,
      config.getInboxCapacity(),
      toConcurrentBufferMode(config.getBufferMode()),
      requiredName(config.getName())}
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
