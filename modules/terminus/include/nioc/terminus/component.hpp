////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msg.hpp"
#include "msgBase.hpp"
#include "port.hpp"
#include "routine.hpp"
#include <boost/circular_buffer.hpp>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <unordered_map>
#include <utility>

namespace nioc::terminus
{

enum class OverflowPolicy : std::uint8_t
{
  Overwrite,
  Block
};

class Component: public Routine
{
public:
  using ChannelId = chronicle::ChannelId;

  Component(const Component&) = delete;
  Component(Component&&) noexcept = delete;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&) noexcept = delete;
  ~Component() noexcept override = default;

  void push(ChannelId channelId, ConstMsgBasePtr msgBasePtr);

  [[nodiscard]] State step() override;

protected:
  using MsgBaseCallback = std::function<void(ConstMsgBasePtr)>;

  template<typename Schema>
  using MsgCallback = std::function<void(ConstMsgPtr<Schema>)>;

  Component(Port& port, std::size_t inboxCapacity, OverflowPolicy overflowPolicy);

  template<typename Schema>
  void subscribe(const std::string_view& topic, MsgCallback<Schema> msgCallback)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);

    // Strictly disallow duplicate subscriptions. Any necessary fan-out must be handled by the
    // passed lamda passed-in by the user.
    if(mHandlers.contains(channelId) or mPortSubscriptions.contains(channelId))
    {
      common::throwException<std::logic_error>(
          "Subscription already exists. Topic: {}, Schema: {}, ChannelId: {}",
          topic,
          common::prettyName<Schema>(),
          channelId.mValue);
    }

    // Set up dispatch pathway from component's queue to the user-provided callback
    mHandlers.emplace(
        channelId,
        [msgCallback = std::move(msgCallback)](ConstMsgBasePtr msgBasePtr)
        {
          std::invoke(
              msgCallback,
              std::static_pointer_cast<const Msg<Schema>>(std::move(msgBasePtr)));
        });

    // Set up dispatch pathway from port to component's queue
    auto msgBaseCallbackPtr = std::make_shared<MsgBaseCallback>(
        [this, channelId](ConstMsgBasePtr msgBasePtr)
        {
          push(channelId, std::move(msgBasePtr));
        });

    mPort.subscribe(channelId, msgBaseCallbackPtr);
    mPortSubscriptions.emplace(channelId, std::move(msgBaseCallbackPtr));
  }

  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  using Inbox = boost::circular_buffer<std::pair<ChannelId, ConstMsgBasePtr>>;

  Port& mPort;

  std::mutex mInboxMutex;
  Inbox mInbox;
  std::condition_variable mInboundConditionVar;
  bool mNotifyOnPush{ false };

  const OverflowPolicy mOverflowPolicy;

  std::unordered_map<ChannelId, MsgBaseCallback> mHandlers;
  std::unordered_map<ChannelId, std::shared_ptr<const MsgBaseCallback>> mPortSubscriptions;
};

} // namespace nioc::terminus
