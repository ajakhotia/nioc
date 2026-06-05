////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msg.hpp"
#include "msgBase.hpp"
#include "port.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/concurrent/notifyingInbox.hpp>
#include <nioc/concurrent/routine.hpp>
#include <nioc/logger/logger.hpp>
#include <unordered_map>
#include <utility>

namespace nioc::terminus
{

/// @brief A @ref Routine that receives subscribed messages through a bounded inbox and may publish.
///
/// A Component is the worker that reacts to data flowing through a @ref Port: a subclass calls @ref
/// subscription to register a typed callback per topic, and the Port delivers matching messages
/// into the Component's inbox. Each @ref step pops one queued message and runs its callback on the
/// Runner's thread, so callbacks never run concurrently with one another. A subclass may also @ref
/// publish to emit messages. Contrast @ref Driver, which only publishes.
///
/// The inbox's storage discipline (concurrent::BufferMode) and capacity are fixed at
/// construction. Construct through a subclass. The Component holds the Port by reference, so the
/// Port must outlive every Component bound to it.
class Component: public concurrent::Routine
{
public:
  using ChannelId = chronicle::ChannelId;

  Component(const Component&) = delete;
  Component(Component&&) noexcept = delete;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&) noexcept = delete;
  ~Component() noexcept override = default;

  /// @brief Pops one queued message and runs its subscribed callback.
  ///
  /// @return @ref State::Continue after handling a message, or @ref State::Waiting when the inbox
  /// is empty.
  [[nodiscard]] State step() override;

protected:
  using MsgBaseCallback = std::function<void(ConstMsgBasePtr)>;
  using ConsignmentCallback = std::function<void(Consignment)>;

  template<typename Schema>
  using MsgCallback = std::function<void(ConstMsgPtr<Schema>)>;

  /// @brief Binds the component to its Port and sizes the inbox.
  ///
  /// @param port Hub the component subscribes to and publishes onto; must outlive this component.
  ///
  /// @param inboxCapacity Maximum number of undelivered messages held at once for a bounded
  /// @p bufferMode; ignored when unbounded.
  ///
  /// @param bufferMode Storage discipline of the inbox (see @ref concurrent::BufferMode).
  ///
  /// @param name Human-readable identity for this component (see @ref Routine::name); a subclass
  /// passes its own type name.
  Component(
      Port& port,
      std::size_t inboxCapacity,
      concurrent::BufferMode bufferMode,
      std::string name);

  /// @brief Subscribes a typed callback to a topic.
  ///
  /// Registers @p msgCallback to receive every message of @p Schema published on the @p topic.
  /// Messages are queued in the inbox, and the callback runs from the @ref step, never
  /// concurrently. A topic may be subscribed at most once; any fan-out is the callback's
  /// responsibility.
  ///
  /// @tparam Schema Cap'n Proto schema of the subscribed message.
  ///
  /// @param topic Topic to subscribe to.
  ///
  /// @param msgCallback Handler invoked with each received message.
  ///
  /// @throws std::logic_error If this topic is already subscribed.
  template<typename Schema>
  void subscribe(const std::string_view& topic, MsgCallback<Schema> msgCallback)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);

    // Strictly disallow duplicate subscriptions. Any necessary fan-out must be handled by the
    // lambda passed in by the user.
    if(mHandlers.contains(channelId) or mPortSubscriptions.contains(channelId))
    {
      common::throwException<std::logic_error>(
          "[{}] Subscription already exists. Topic: {}, Schema: {}, ChannelId: {}",
          name(),
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

    // Set up dispatch pathway for the consignments from port to component's queue
    auto consignmentCallbackPtr = std::make_shared<ConsignmentCallback>(
        [this, channelId](Consignment consignment)
        {
          push(channelId, std::move(consignment));
        });

    mPort.subscribe(channelId, consignmentCallbackPtr);
    mPortSubscriptions.emplace(channelId, std::move(consignmentCallbackPtr));

    logger::debug(
        "[{}] subscribed to topic '{}' (schema {}, channel {})",
        name(),
        topic,
        common::prettyName<Schema>(),
        channelId.mValue);
  }

  /// @brief Publishes a typed message onto the bound Port.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic the message is published on.
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish(makeChannelId(Msg<Schema>::kMsgId, topic), std::move(msgPtr));
  }

private:
  Port& mPort;
  concurrent::NotifyingInbox<concurrent::AnyMpsc<std::pair<ChannelId, Consignment>>> mInbox;
  std::unordered_map<ChannelId, MsgBaseCallback> mHandlers;
  std::unordered_map<ChannelId, std::shared_ptr<const ConsignmentCallback>> mPortSubscriptions;

  /// @brief Enqueues a consignment for the channel.
  ///
  /// @param channelId Channel the message arrived on.
  ///
  /// @param consignment Consignment to enqueue.
  void push(ChannelId channelId, Consignment consignment);
};

} // namespace nioc::terminus
