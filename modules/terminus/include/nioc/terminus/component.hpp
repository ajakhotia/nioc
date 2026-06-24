////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "consignment.hpp"
#include "message.hpp"
#include "port.hpp"
#include "publisher.hpp"
#include <cstddef>
#include <functional>
#include <nioc/chronicle/defines.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/concurrent/notifyingInbox.hpp>
#include <nioc/concurrent/routine.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/config/componentConfig.capnp.h>
#include <nioc/terminus/schemaId.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace nioc::terminus
{

/// @brief A @ref Routine that receives messages through an inbox and may publish.
///
/// A subclass calls @ref subscribe to register a typed callback per topic; the Port queues matching
/// frames in the inbox, and each @ref step runs one queued callback on the Runner's thread, so
/// callbacks never run at the same time. A subclass that also produces holds @ref Publisher handles
/// minted with @ref publisher at construction. Compare @ref Driver, which only publishes.
///
/// Construct through a subclass. The Port is held by reference and must outlive every Component.
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
  using ConsignmentHandler = std::function<State(Consignment)>;

  template<typename Schema>
  using MessageCallback = std::function<State(const Message<Schema>&)>;

  /// @brief Binds the component to its Port and sizes the inbox.
  ///
  /// @param port Port to subscribe to and publish on; must outlive this component.
  ///
  /// @param inboxCapacity Max messages held at once when @p bufferMode is bounded; ignored when
  /// unbounded.
  ///
  /// @param bufferMode Inbox buffer mode (see @ref concurrent::BufferMode).
  ///
  /// @param name Name of this component (see @ref Routine::name).
  Component(
      Port& port,
      std::size_t inboxCapacity,
      concurrent::BufferMode bufferMode,
      std::string name);

  /// @brief Configures the component base from its config block.
  ///
  /// @param port Port to subscribe to and publish on; must outlive this component.
  ///
  /// @param config The component's config block (see componentConfig.capnp).
  ///
  /// @throws std::invalid_argument If the name is empty; the config must provide it.
  Component(Port& port, ComponentConfig::Reader config);

  /// @brief Mints a producer handle for a topic.
  ///
  /// @tparam Schema Cap'n Proto schema of the messages.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @return A @ref Publisher bound to the topic.
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    return mPort.publisher<Schema>(topic);
  }

  /// @brief Subscribes a typed callback to a topic.
  ///
  /// @p messageCallback receives every @p Schema message published on @p topic. Messages queue in
  /// the inbox and the callback runs from @ref step, never concurrently. Subscribe a topic at most
  /// once; any fan-out is the callback's job.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to subscribe to.
  ///
  /// @param messageCallback Called with each message. The @ref State it returns becomes the
  /// component's next state, so returning @ref State::Done finishes the component.
  ///
  /// @throws std::logic_error If this topic is already subscribed.
  template<typename Schema>
  void subscribe(const std::string_view& topic, MessageCallback<Schema> messageCallback)
  {
    const auto channelId = chronicle::makeChannelId(kSchemaId<Schema>, topic);

    // No duplicate subscriptions. Fan-out is the user's callback's job.
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
    // clang-format off
    const auto& handler = mHandlers.emplace(
      channelId,
      [messageCallback = std::move(messageCallback)](Consignment consignment) -> State
      {
        const auto message = Message<Schema>{consignment.crate()};
        return std::invoke(messageCallback, message);
      }).first->second;
    // clang-format on

    // Port-to-inbox step. Pushes the frame with a pointer to its handler.
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

private:
  using MpscQueue = concurrent::AnyMpsc<std::pair<const ConsignmentHandler*, Consignment>>;

  Port& mPort;
  concurrent::NotifyingInbox<MpscQueue> mInbox;
  std::unordered_map<ChannelId, ConsignmentHandler> mHandlers;

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
