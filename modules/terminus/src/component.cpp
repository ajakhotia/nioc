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

void Component::push(const ChannelId channelId, ConstMsgBasePtr msgBasePtr)
{
  logger::trace("[{}] inbox enqueue on channel {}", name(), channelId.mValue);
  mInbox.push(std::make_pair(channelId, std::move(msgBasePtr)));
}

Component::State Component::step()
{
  auto value = mInbox.tryPop();
  if(not value)
  {
    return State::Waiting;
  }

  if(mHandlers.contains(value->first))
  {
    logger::trace("[{}] handling channel {}", name(), value->first.mValue);
    std::invoke(mHandlers.at(value->first), std::move(value->second));
  }
  else
  {
    logger::trace("[{}] no handler for channel {}; dropping", name(), value->first.mValue);
  }

  return State::Continue;
}

} // namespace nioc::terminus
