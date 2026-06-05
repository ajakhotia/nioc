////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

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
  mInbox(
      [this]
      {
        wakeRunner();
      },
      bufferMode,
      inboxCapacity)
{
}

Component::State Component::step()
{
  auto value = mInbox.tryPop();
  if(not value)
  {
    return State::Waiting;
  }

  // Dispatch moves the message into the handler. When this consignment is later destroyed, its
  // RaiiToken runs Release, decrementing the port's in-flight counter to report the delivery.
  std::invoke(mHandlers.at(value->first), std::move(value->second.mMsgBasePtr));

  return State::Continue;
}

void Component::push(ChannelId channelId, Consignment consignment)
{
  logger::trace("[{}] inbox enqueue on channel {}", name(), channelId.mValue);
  mInbox.push({channelId, std::move(consignment)});
}

} // namespace nioc::terminus
