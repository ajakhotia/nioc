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
#include <nioc/terminus/config/componentConfig.capnp.h>
#include <unordered_map>
#include <utility>

namespace nioc::terminus
{

/// @brief A @ref Routine that receives messages through an inbox and may publish.
///
/// A Component reacts to data on a @ref Port. A subclass calls @ref subscribe to register a typed
/// callback per topic; the Port queues matching messages in the inbox. Each @ref step runs one
/// queued callback on the Runner's thread, so callbacks never run at the same time. A subclass may
/// also @ref publish. Compare @ref Driver, which only publishes.
///
/// Inbox buffer mode and capacity are fixed at construction. Construct through a subclass. The Port
/// is held by reference and must outlive every Component bound to it.
class Component: public concurrent::Routine
{
public:
  using ChannelId = chronicle::ChannelId;

  Component(const Component&) = delete;
  Component(Component&&) noexcept = delete;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&) noexcept = delete;
  ~Component() noexcept override = default;

protected:
  using MsgBaseCallback = std::function<State(ConstMsgBasePtr)>;

  template<typename Schema>
  using MsgCallback = std::function<State(ConstMsgPtr<Schema>)>;

  /// @brief Binds the component to its Port and sizes the inbox.
  ///
  /// @param port Port to subscribe to and publish on; must outlive this component.
  ///
  /// @param inboxCapacity Max messages held at once when @p bufferMode is bounded; ignored when
  /// unbounded.
  ///
  /// @param bufferMode Inbox buffer mode (see @ref concurrent::BufferMode).
  ///
  /// @param name Name of this component (see @ref Routine::name); a subclass passes its own type
  /// name.
  Component(
      Port& port,
      std::size_t inboxCapacity,
      concurrent::BufferMode bufferMode,
      std::string name);

  /// @brief Configures the component base from its config block.
  ///
  /// Reads the name (see @ref Routine::name) and inbox settings from @p config. A subclass passes
  /// the `component` subsection of its own config block here.
  ///
  /// @param port Port to subscribe to and publish on; must outlive this component.
  ///
  /// @param config The component's config block (see componentConfig.capnp).
  ///
  /// @throws std::invalid_argument If the name is empty; the config must provide it.
  Component(Port& port, ComponentConfig::Reader config);

  /// @brief Subscribes a typed callback to a topic.
  ///
  /// @p msgCallback receives every @p Schema message published on @p topic. Messages are queued in
  /// the inbox and the callback runs from @ref step, never concurrently. Subscribe a topic at most
  /// once; any fan-out is the callback's job.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to subscribe to.
  ///
  /// @param msgCallback Called with each message. The @ref State it returns becomes the component's
  /// next state, so returning @ref State::Done finishes the component.
  ///
  /// @throws std::logic_error If this topic is already subscribed.
  template<typename Schema>
  void subscribe(const std::string_view& topic, MsgCallback<Schema> msgCallback)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);

    // No duplicate subscriptions. Fan-out is the user's lambda's job.
    if(mHandlers.contains(channelId))
    {
      common::throwException<std::logic_error>(
          "[{}] Subscription already exists. Topic: {}, Schema: {}, ChannelId: {}",
          name(),
          topic,
          common::prettyName<Schema>(),
          channelId.mValue);
    }

    // Inbox-to-callback step. The handler lives at a stable address (an unordered_map never moves
    // an element), so the inbox can carry a pointer straight to it.
    const auto& handler =
        mHandlers
            .emplace(
                channelId,
                [msgCallback = std::move(msgCallback)](ConstMsgBasePtr msgBasePtr) -> State
                {
                  return std::invoke(
                      msgCallback,
                      std::static_pointer_cast<const Msg<Schema>>(std::move(msgBasePtr)));
                })
            .first->second;

    // Port-to-inbox step. Pushes the message with a pointer to its handler.
    mPort.subscribe(
        channelId,
        [this, handlerPtr = &handler](Consignment consignment)
        { mInbox.push({handlerPtr, std::move(consignment)}); });

    logger::info(
        "[{}] subscribed to topic '{}' (schema {}, channel id {}).",
        name(),
        topic,
        common::prettyName<Schema>(),
        channelId.mValue);
  }

  /// @brief Publishes a typed message on the bound Port.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  using MpscQueue = concurrent::AnyMpsc<std::pair<const MsgBaseCallback*, Consignment>>;

  Port& mPort;
  concurrent::NotifyingInbox<MpscQueue> mInbox;
  std::unordered_map<ChannelId, MsgBaseCallback> mHandlers;

  /// @brief Pops one queued message and runs its callback, returning the next State.
  ///
  /// Catches any exception the callback throws, logs it, and returns @ref State::Done so a failing
  /// component shuts down cleanly. Never throws.
  ///
  /// @return @ref State::Waiting if the inbox is empty; the @ref State from the callback otherwise;
  /// or @ref State::Done if the callback throws.
  [[nodiscard]] State step() noexcept final;
};

} // namespace nioc::terminus
