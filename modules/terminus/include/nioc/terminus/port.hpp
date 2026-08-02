////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "config.hpp"
#include "consignment.hpp"
#include "runContext.hpp"
#include "schemaId.hpp"
#include "schemaRegistry.hpp"
#include "topicRegistry.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/locked.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nioc/concurrent/runner.hpp>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nioc::terminus
{

class Component;
class Driver;

template<typename Schema_>
class Publisher;

/// @brief The hub of one recording run: it owns the run's working directory, logging, chronicle,
/// publish/subscribe bus, routine graph, and shutdown/abort signals.
///
/// A Port's lifetime is the run's lifetime. Construction stamps out a unique working directory,
/// attaches a console log and (when the run records) a chronicle writer, then runs the @ref Setup
/// hook to build the graph of drivers, components, and runners. Destruction winds the run down and
/// tears the graph down in dependency order. Routines hold a reference to their Port and use it to
/// publish, subscribe, acquire resources, and observe the shutdown/abort tokens.
///
/// Neither copyable nor movable; pass by reference. One Port is shared across the run's worker
/// threads. `addResource`/`acquireResource`, `deliver`, `shutdown`, and `abort` are thread-safe;
/// `publisher` and `subscribe` are wiring-time operations and are not safe against concurrent
/// delivery.
///
/// @see Publisher, Consignment, RunContext
class Port
{
public:
  /// Identifies one publish/subscribe channel; derived from a `(Schema, topic)` pair.
  using ChannelId = chronicle::ChannelId;

  /// Subscriber invoked once per delivered consignment, synchronously on the publishing thread.
  using ConsignmentCallback = std::function<void(Consignment)>;

  /// The run's data sources, torn down first.
  using Drivers = std::vector<std::shared_ptr<Driver>>;

  /// The run's processing nodes, torn down after drivers.
  using Components = std::vector<std::shared_ptr<Component>>;

  /// The run's thread runners, torn down last.
  using Runners = std::vector<std::shared_ptr<concurrent::Runner>>;

  /// @brief Wiring hook that builds the run's routine graph.
  ///
  /// Called once by the constructor against the fully initialized Port. Push the run's drivers,
  /// components, and runners into the matching vectors and register their subscriptions. The graph
  /// is later torn down in order: drivers, then components, then runners.
  using Setup = std::function<void(Port&, Drivers&, Components&, Runners&)>;

  /// @brief Create the run: attach logging and the chronicle writer, copy in the run's resources,
  /// then call @p setup to wire the routine graph.
  ///
  /// The working directory is the one @p runContext owns and has already established (its
  /// `configOverlay.json` and `manifest.json` are written by then). Port writes `resources.json`
  /// into it. @p setup runs against the fully constructed Port.
  ///
  /// @param runContext Owns the working directory and carries the config layers, resource files,
  /// record/playback mode, and command line. Consumed (moved in).
  ///
  /// @param setup Wiring hook run against the fully constructed Port to build the routine graph.
  ///
  /// @throws std::invalid_argument if a resource file is missing, is not a regular file, or
  /// collides with another resource by full path or by filename.
  ///
  /// @throws std::runtime_error if a config file cannot be opened.
  Port(RunContext runContext, const Setup& setup);

  Port(const Port&) = delete;

  Port(Port&&) noexcept = delete;

  /// @brief Destructor. Request shutdown, let the system wind down and terminate cleanly.
  ~Port() noexcept;

  Port& operator=(const Port&) = delete;

  Port& operator=(Port&&) noexcept = delete;

  /// Root directory holding this run's chronicle, console log, and copied resources.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// Read-only view of how this run was launched: working directory, resources, config layers,
  /// record/playback mode, and command line.
  ///
  /// @see RunContext
  [[nodiscard]] const RunContext& runContext() const noexcept;

  /// @brief The topics of the recording this run replays.
  ///
  /// Adopted before the @ref Setup hook runs, so a routine may consult it while wiring its
  /// subscriptions. Empty on a run that is not a replay. To read a recording's topics without
  /// standing up a run, construct a @ref TopicRegistry from its directory directly.
  [[nodiscard]] const TopicRegistry& playbackTopics() const noexcept;

  /// @brief The schemas of the recording this run replays, as live schemas for dynamic reading.
  ///
  /// Adopted before the @ref Setup hook runs, so a routine may consult it while wiring its
  /// subscriptions; join it to channels through the schema ids in @ref playbackTopics. Empty on a
  /// run that is not a replay.
  [[nodiscard]] const SchemaRegistry& playbackSchemas() const noexcept;

  /// @brief Materialize the typed @ref Config for the routine named @p name: read its overrides
  /// from this run's @ref ConfigOverlay, merge them onto @p Schema's defaults, and write and map
  /// the config artifacts under the run's `config` directory.
  ///
  /// A routine's `(name, port)` constructor delegates through this to its `(name, port, config)`
  /// constructor. The routine names only itself; where its overrides live in the config document is
  /// the ConfigOverlay's concern, not the routine's.
  ///
  /// @tparam Schema The routine's Cap'n Proto config schema.
  ///
  /// @param name The routine name; keys its overrides and names its artifacts.
  ///
  /// @throws std::invalid_argument if the overrides are not a JSON object.
  ///
  /// @throws std::runtime_error if an artifact cannot be written or mapped.
  template<typename Schema>
  [[nodiscard]] Config<Schema> materializeConfig(const std::string& name)
  {
    return Config<Schema>{
        mRunContext.configOverlay().acquireOverrides(name),
        mRunContext.workingDir() / "config",
        name};
  }

  /// @brief Copy @p source into the working directory and register it as a run resource.
  ///
  /// Thread-safe.
  ///
  /// @param source An existing regular file.
  ///
  /// @throws std::invalid_argument if @p source is missing, is not a regular file, or collides by
  /// full path or by filename with an already added resource.
  void addResource(const std::filesystem::path& source);

  /// @brief Return the working-directory path of @p source, copying it in on first request.
  ///
  /// Idempotent: repeated calls for the same @p source return the same path without re-copying.
  /// Thread-safe.
  ///
  /// @throws std::invalid_argument on the first (copying) call if @p source is missing, is not a
  /// regular file, or collides by filename with another resource.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source);

  /// @brief Return the working-directory path of an already added @p source; never copies.
  ///
  /// Thread-safe.
  ///
  /// @throws std::out_of_range if @p source was not previously added.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

  /// @brief Open a publisher for @p topic carrying messages of @p Schema, recording the topic to
  /// the run's `topics.json` and the schema's closure to its `schemas.bin`.
  ///
  /// A `(Schema, topic)` pair is one channel, and a channel takes a single publisher: chronicle
  /// channels are single-producer, so opening a second publisher for a channel already opened on
  /// this run is refused. Call at wiring time.
  ///
  /// @tparam Schema The Cap'n Proto payload schema. Must be supplied explicitly.
  ///
  /// @throws std::logic_error if this run does not record a chronicle.
  ///
  /// @throws std::runtime_error if a publisher is already open for this channel.
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    if(mWriter == nullptr)
    {
      common::throwException<std::logic_error>(
          "Port::publisher requires a recording run; this run does not record");
    }

    const auto channelId = chronicle::makeChannelId(kSchemaId<Schema>, topic);
    mActiveTopicRegistry.record(
        channelId,
        std::string{topic},
        kSchemaId<Schema>,
        std::string{common::prettyName<Schema>()});
    mActiveSchemaRegistry.record<Schema>();
    return Publisher<Schema>{*this, mWriter->channel(channelId)};
  }

  /// @brief Register @p callback to receive every crate delivered on @p channelId.
  ///
  /// Multiple callbacks may subscribe to one channel; each is invoked in registration order. Call
  /// at wiring time, before delivery begins. Not synchronized against concurrent @ref deliver.
  void subscribe(ChannelId channelId, ConsignmentCallback callback);

  /// @brief Fan @p crate out to every subscriber of @p channelId, synchronously on the calling
  /// thread.
  ///
  /// Each callback receives a fresh Consignment that holds the run back from quiescence for as long
  /// as the callback (or anything it hands the consignment to) keeps it alive. Channels with no
  /// subscribers are dropped silently. Usually called by a Publisher, not directly.
  ///
  /// @see Consignment, awaitQuiescence
  void deliver(ChannelId channelId, const chronicle::Crate& crate) const;

  /// @brief Request a graceful stop: signal the shutdown token so producers finish and the run
  /// winds down.
  ///
  /// Idempotent and thread-safe; returns immediately without waiting for the run to stop.
  void shutdown() const noexcept;

  /// @brief Request an immediate abort: signal both the shutdown and abort tokens and release any
  /// thread blocked in @ref awaitQuiescence, abandoning still in-flight consignments.
  ///
  /// Idempotent and thread-safe; returns immediately.
  void abort() const noexcept;

  /// Token stopped on @ref shutdown (and @ref abort); poll or register a callback to drive a
  /// routine's graceful exit.
  [[nodiscard]] std::stop_token shutdownToken() const noexcept;

  /// Token stopped only on @ref abort; signals that in-flight work should be abandoned.
  [[nodiscard]] std::stop_token abortToken() const noexcept;

  /// @brief Block until every in-flight consignment has been destroyed, or @ref abort is requested.
  ///
  /// Returns immediately when nothing is in flight. Call after @ref shutdown to drain the bus
  /// before tearing routines down.
  void awaitQuiescence() const;

  /// @brief Run @p housekeeping once, then sleep until @p duration elapses unless every driver has
  /// finished.
  ///
  /// Drive a polling supervisor loop by calling this until it returns `false`. @p duration is
  /// measured from entry, before @p housekeeping runs.
  ///
  /// @return `false` once all drivers reach the Done state (no sleep is performed); otherwise
  /// sleeps to the deadline and returns `true`.
  [[nodiscard]] bool wait(
      std::chrono::nanoseconds duration,
      const std::function<void()>& housekeeping) const;

private:
  /// Maps each added resource's source path to its filename inside the working directory.
  using ResourceMap = std::unordered_map<std::string, std::string>;

  /// The subscribers on one channel, invoked in registration order.
  using SubscriptionList = std::vector<ConsignmentCallback>;

  /// Maps each subscribed channel to its list of subscribers.
  using SubscriptionMap = std::unordered_map<ChannelId, SubscriptionList>;

  /// How this run was launched: working directory, resources, config layers, mode, and the
  /// assembled config overlay. Owns the working directory; declared first so it is built before the
  /// members that read from it.
  const RunContext mRunContext;

  /// The console log sink attached at construction and detached during teardown.
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;

  /// The chronicle writer for a recording run; null when the run does not record.
  const std::unique_ptr<chronicle::Writer> mWriter;

  /// The added resources, guarded for concurrent @ref addResource and @ref acquireResource.
  common::Locked<ResourceMap> mLockedResourceMap;

  /// The topics of the recording this run replays. Adopted in the initializer list from the input
  /// log, so it is ready before the graph is wired; empty on a run that is not a replay. Never
  /// written back out, since it belongs to the recording being read, not this run.
  const TopicRegistry mPlaybackTopicRegistry;

  /// The schemas of the recording this run replays. Adopted in the initializer list from the input
  /// log, like @ref mPlaybackTopicRegistry, and never written back out.
  const SchemaRegistry mPlaybackSchemaRegistry;

  /// The topics this run itself records, filled as its publishers open and written out once as
  /// `topics.json` when the graph is wired.
  TopicRegistry mActiveTopicRegistry;

  /// The schema closures of the topics this run records, filled beside @ref mActiveTopicRegistry
  /// and written out once as `schemas.bin` when the graph is wired.
  SchemaRegistry mActiveSchemaRegistry;

  /// The subscribers registered per channel, consulted by @ref deliver.
  SubscriptionMap mSubscriptionMap;

  /// The count of consignments still in flight; drives @ref awaitQuiescence.
  mutable std::atomic_uint32_t mPendingConsignments{0};

  /// The source behind @ref shutdownToken, signalled by @ref shutdown and @ref abort.
  std::stop_source mShutdownSource;

  /// The source behind @ref abortToken, signalled by @ref abort.
  std::stop_source mAbortSource;

  // The run's routine graph. Declared in this order, so natural member destruction runs drivers →
  // components → runners, matching the destructor's teardown order.
  Runners mRunners;
  Components mComponents;
  Drivers mDrivers;
};

} // namespace nioc::terminus
