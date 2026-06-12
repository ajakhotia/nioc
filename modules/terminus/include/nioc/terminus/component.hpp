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

protected:
  using MsgBaseCallback = std::function<State(ConstMsgBasePtr)>;

  template<typename Schema>
  using MsgCallback = std::function<State(ConstMsgPtr<Schema>)>;

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

  /// @brief Configures the component base from its config block.
  ///
  /// Reads the routine's name (see @ref Routine::name; conventionally matching the tag of the
  /// subclass's config block) and the inbox settings out of @p config. By convention a subclass
  /// forwards the `component` subsection of its own config block here, keeping the base's settings
  /// out of the subclass's namespace.
  ///
  /// @param port Hub the component subscribes to and publishes onto; must outlive this component.
  ///
  /// @param config View of the component's config block (see componentConfig.capnp).
  ///
  /// @throws std::invalid_argument If the name is empty: it is instance-specific, so the config
  /// data must provide it.
  Component(Port& port, ComponentConfig::Reader config);

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
  /// @param msgCallback Handler invoked with each received message; the @ref State it returns
  /// becomes the component's next state, so returning @ref State::Done finishes the component.
  ///
  /// @throws std::logic_error If this topic is already subscribed.
  template<typename Schema>
  void subscribe(const std::string_view& topic, MsgCallback<Schema> msgCallback)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);

    // Strictly disallow duplicate subscriptions. Any necessary fan-out must be handled by the
    // lambda passed in by the user.
    if(mHandlers.contains(channelId))
    {
      common::throwException<std::logic_error>(
          "[{}] Subscription already exists. Topic: {}, Schema: {}, ChannelId: {}",
          name(),
          topic,
          common::prettyName<Schema>(),
          channelId.mValue);
    }

    // Dispatch pathway from the inbox to the user-provided callback. The handler is stored at a
    // stable address (an unordered_map never relocates an element), and the inbox carries a pointer
    // straight to it.
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

    // Dispatch pathway from port to the component's inbox. This also contains a pointer to
    // the handler that will take the msg from the inbox to the user-provided callback.
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
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  using MpscQueue = concurrent::AnyMpsc<std::pair<const MsgBaseCallback*, Consignment>>;

  Port& mPort;
  concurrent::NotifyingInbox<MpscQueue> mInbox;
  std::unordered_map<ChannelId, MsgBaseCallback> mHandlers;

  /// @brief Pops one queued message and runs its subscribed callback, reporting the next State.
  ///
  /// Catches every exception a callback may throw, logs it, and reports @ref State::Done so a
  /// failing component winds down gracefully rather than escalating. Never throws.
  ///
  /// @return @ref State::Waiting when the inbox is empty; otherwise the @ref State returned by the
  /// subscribed callback; or @ref State::Done if the callback throws.
  [[nodiscard]] State step() noexcept final;
};

} // namespace nioc::terminus
