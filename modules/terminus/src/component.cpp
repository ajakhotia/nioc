////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <nioc/terminus/component.hpp>
#include <optional>
#include <utility>

namespace nioc::terminus
{

Component::Component(
    Port& port,
    const std::size_t inboxCapacity,
    const OverflowPolicy overflowPolicy):
    mPort(port),
    mInbox(
        inboxCapacity,
        overflowPolicy,
        [this]
        {
          notifyReady();
        })
{
}

void Component::push(const ChannelId channelId, ConstMsgBasePtr msgBasePtr)
{
  mInbox.push_back(std::make_pair(channelId, std::move(msgBasePtr)));
}

Component::State Component::step()
{
  auto value = mInbox.try_pop();
  if(not value)
  {
    return State::Waiting;
  }

  if(mHandlers.contains(value->first))
  {
    std::invoke(mHandlers.at(value->first), std::move(value->second));
  }

  return State::Continue;
}

} // namespace nioc::terminus
