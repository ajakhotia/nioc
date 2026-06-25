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

/// @brief A message-driven processing node: subscribe to topics, react to messages, publish
/// results.
///
/// Derive from `Component`, then in your constructor open publishers with `publisher` and register
/// callbacks with `subscribe`. The driving `Runner` ticks the component, pulling one queued message
/// off its inbox per tick and invoking the matching callback. Deliveries are processed serially on
/// the component's own tick, so your callbacks never run concurrently with each other, no matter
/// which thread the `Port` delivered from.
///
/// Example:
///
///     class Echo: public Component
///     {
///     public:
///       explicit Echo(Port& port): Component(port, 64, BufferMode::Overwriting, "echo")
///       {
///         mOut = publisher<MySchema>("out");
///         subscribe<MySchema>(
///             "in",
///             [this](const Message<MySchema>& msg)
///             {
///               // ... react to msg ...
///               return State::Continue;
///             });
///       }
///     };
///
/// Non-copyable and non-movable. The `Port` passed at construction owns the component and must
/// outlive it. Wiring (`publisher`, `subscribe`) is meant for construction time, before the run
/// starts delivering.
///
/// @see Port, Publisher, Message, concurrent::Routine
class Component: public concurrent::Routine
{
public:
  /// Identifier for one `(schema, topic)` channel on the bus.
  using ChannelId = chronicle::ChannelId;

  Component(const Component&) = delete;
  Component(Component&&) noexcept = delete;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&) noexcept = delete;
  ~Component() noexcept override = default;

protected:
  /// Internal handler the inbox dispatches to: decodes a `Consignment` and runs one subscription.
  using ConsignmentHandler = std::function<State(Consignment)>;

  /// @brief The callback you register for a topic; runs once per delivered message and returns how
  /// the component should proceed.
  ///
  /// Return `State::Continue` to keep the component running, or `State::Done` to stop driving it.
  ///
  /// @tparam Schema The Cap'n Proto schema of the messages this callback receives.
  template<typename Schema>
  using MessageCallback = std::function<State(const Message<Schema>&)>;

  /// @brief Construct with an explicit inbox size, overflow policy, and name.
  ///
  /// @param port The owning port; it must outlive the component.
  ///
  /// @param inboxCapacity Maximum number of pending messages the inbox holds before `bufferMode`
  /// decides what happens.
  ///
  /// @param bufferMode What to do when the inbox is full: `Overwriting` drops the oldest message,
  /// `Unbounded` grows without limit (ignores `inboxCapacity`).
  ///
  /// @param name The routine label used in logs; fixed for the component's life.
  Component(
      Port& port,
      std::size_t inboxCapacity,
      concurrent::BufferMode bufferMode,
      std::string name);

  /// @brief Construct from a Cap'n Proto config reader, taking inbox size, buffer mode, and name
  /// from it.
  ///
  /// @throws std::invalid_argument If the config's name is empty, or its buffer mode is not a
  /// recognized value.
  Component(Port& port, ComponentConfig::Reader config);

  /// @brief Open a publisher that sends messages of `Schema` on `topic`, registering the topic with
  /// the run.
  ///
  /// @tparam Schema The message schema; must be supplied explicitly.
  ///
  /// @param topic Channel name to publish on.
  ///
  /// @returns A publisher bound to the `Port` and its channel; it must not outlive the `Port`.
  ///
  /// @throws std::logic_error If the run does not record a chronicle.
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    return mPort.publisher<Schema>(topic);
  }

  /// @brief Register `messageCallback` to handle messages of `Schema` published on `topic`.
  ///
  /// Each delivery is queued on the component's inbox and the callback runs on the component's own
  /// tick, never on the publishing thread. Call at wiring time, before the run starts delivering.
  ///
  /// @tparam Schema The message schema; must be supplied explicitly.
  ///
  /// @param topic Channel name to subscribe to.
  ///
  /// @param messageCallback Invoked once per delivered message; its returned `State` drives the
  /// component.
  ///
  /// @throws std::logic_error If a callback is already subscribed to this `(Schema, topic)`. At
  /// most one subscription per channel is allowed; fan-out to several
  /// consumers is the callback's responsibility.
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
  /// The inbox's element type: a delivered `Consignment` paired with a borrowed pointer to the
  /// handler that should decode and dispatch it. The pointer aliases an entry in `mHandlers`.
  using MpscQueue = concurrent::AnyMpsc<std::pair<const ConsignmentHandler*, Consignment>>;

  /// The owning `Port`. Borrowed, not owned; it must outlive the component.
  Port& mPort;

  /// Queue of deliveries awaiting dispatch, drained one entry per tick by `step`.
  concurrent::NotifyingInbox<MpscQueue> mInbox;

  /// The registered handlers, keyed by channel. Entries are stable in memory, so `mInbox` may hold
  /// pointers into this map; at most one handler exists per channel.
  std::unordered_map<ChannelId, ConsignmentHandler> mHandlers;

  /// @brief Process one tick: pull the next queued delivery off the inbox and run its handler.
  ///
  /// Called by the driving `Runner`. Runs serially with respect to itself, so handlers never
  /// overlap.
  ///
  /// @returns `State::Waiting` when no delivery is queued; otherwise the `State` the dispatched
  /// handler returns; `State::Done` if a handler throws (the error is logged).
  [[nodiscard]] State step() noexcept final;
};

} // namespace nioc::terminus
