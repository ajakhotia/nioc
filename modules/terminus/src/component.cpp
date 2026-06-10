////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <functional>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/component.hpp>
#include <optional>
#include <utility>

namespace nioc::terminus
{

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

Component::State Component::step() noexcept
{
  try
  {
    auto value = mInbox.tryPop();
    if(not value)
    {
      return State::Waiting;
    }

    // Dispatch hands the message to the subscribed callback, which returns the next State. When
    // `value` leaves scope its Consignment is destroyed, decrementing the port's in-flight counter
    // to report the delivery.
    return std::invoke(*value->first, std::move(value->second.mMsgBasePtr));
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
