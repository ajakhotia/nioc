////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/common/exception.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/component.hpp>
#include <stdexcept>
#include <utility>

namespace nioc::terminus
{

Component::Component(
    Port& port,
    const std::size_t inboxCapacity,
    const OverflowPolicy overflowPolicy):
    mPort(port),
    mInbox(inboxCapacity),
    mOverflowPolicy{ overflowPolicy }
{
  if(mInbox.capacity() < 1)
  {
    common::throwException<std::invalid_argument>("Component inbox capacity must be at least 1.");
  }
}

void Component::push(const ChannelId channelId, ConstMsgBasePtr msgBasePtr)
{
  auto shouldNotify = false;
  {
    auto lock = std::unique_lock(mInboxMutex);

    if(mInbox.size() == mInbox.capacity())
    {
      switch(mOverflowPolicy)
      {
        case OverflowPolicy::Block:
        {
          mInboundConditionVar.wait(
              lock,
              [this]
              {
                return mInbox.size() < mInbox.capacity();
              });
          break;
        }
        case OverflowPolicy::Overwrite:
        {
          logger::debug("Component inbox is full. Overwriting oldest message.");
          break;
        }
      }
    }

    mInbox.push_back({ channelId, std::move(msgBasePtr) });
    shouldNotify = std::exchange(mNotifyOnPush, false);
  }

  if(shouldNotify)
  {
    notifyReady();
  }
}

Component::State Component::step()
{
  auto channelMsgPair = std::pair<ChannelId, ConstMsgBasePtr>();
  {
    const auto lock = std::scoped_lock(mInboxMutex);
    if(mInbox.empty())
    {
      mNotifyOnPush = true;
      return State::Waiting;
    }

    std::swap(channelMsgPair, mInbox.front());
    mInbox.pop_front();
    mInboundConditionVar.notify_one();
  }

  if(mHandlers.contains(channelMsgPair.first))
  {
    std::invoke(mHandlers.at(channelMsgPair.first), std::move(channelMsgPair.second));
  }

  return State::Continue;
}

} // namespace nioc::terminus
