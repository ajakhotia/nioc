////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "configStore.hpp"
#include "consignment.hpp"
#include "manifest.hpp"
#include "runContext.hpp"
#include "schemaId.hpp"
#include <atomic>
#include <chrono>
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
#include <unordered_set>
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
/// @see Publisher, Consignment, Manifest
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

  /// @brief Create the run: build its working directory, attach logging and the chronicle writer,
  /// copy in the manifest's resources, then call @p setup to wire the routine graph.
  ///
  /// Writes the manifest and a `resources.json` into the working directory. @p setup runs against
  /// the fully constructed Port.
  ///
  /// @param manifest Defines the log root, whether to record a chronicle, and the resource files
  /// to copy in.
  ///
  /// @param setup Wiring hook run against the fully constructed Port to build the routine graph.
  ///
  /// @throws std::invalid_argument if a resource file is missing, is not a regular file, or
  /// collides with another resource by full path or by filename.
  explicit Port(Manifest manifest, const Setup& setup);

  Port(const Port&) = delete;

  Port(Port&&) noexcept = delete;

  /// @brief Wind the run down: request shutdown, drain in-flight consignments, release the routine
  /// graph in dependency order, rewrite `resources.json`, then detach logging.
  ///
  /// The chronicle writer is finalized last, after every crate viewing its rolls is gone. Does not
  /// throw.
  ~Port();

  Port& operator=(const Port&) = delete;

  Port& operator=(Port&&) noexcept = delete;

  /// Root directory holding this run's chronicle, console log, and copied resources.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// Read-only view of how this run was launched: log root, resources, record/playback mode, and
  /// command line. For the decoded config, use @ref config.
  ///
  /// @see RunContext
  [[nodiscard]] const RunContext& runContext() const noexcept;

  /// @brief Return a typed reader over the run's decoded configuration.
  ///
  /// The reader borrows from this Port and must not outlive it.
  ///
  /// @tparam Schema Must be supplied explicitly and match the schema the config was decoded
  /// against.
  ///
  /// @throws std::logic_error if the run's config was built without a schema.
  template<typename Schema>
  [[nodiscard]] Schema::Reader config() const
  {
    return mManifest.mConfigStore.get<Schema>();
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
  /// the run's `topics.txt`.
  ///
  /// Each distinct `(Schema, topic)` pair is one channel; calling again with the same pair yields
  /// an independent publisher onto the same channel. Call at wiring time.
  ///
  /// @tparam Schema The Cap'n Proto payload schema. Must be supplied explicitly.
  ///
  /// @throws std::logic_error if this run does not record a chronicle.
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    if(mWriter == nullptr)
    {
      common::throwException<std::logic_error>(
          "Port::publisher requires a recording run; this run does not record");
    }

    const auto channelId = chronicle::makeChannelId(kSchemaId<Schema>, topic);
    recordTopic(channelId, topic, common::prettyName<Schema>());
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

  /// The set of channels already recorded to `topics.txt`.
  using ChannelIdSet = std::unordered_set<ChannelId>;

  /// How this run was launched: log root, resources, record/playback mode, and config.
  const Manifest mManifest;

  /// Root directory holding this run's chronicle, console log, and copied resources.
  const std::filesystem::path mWorkingDir;

  /// The console log sink attached at construction and detached during teardown.
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;

  /// The chronicle writer for a recording run; null when the run does not record.
  const std::unique_ptr<chronicle::Writer> mWriter;

  /// The added resources, guarded for concurrent @ref addResource and @ref acquireResource.
  common::Locked<ResourceMap> mLockedResourceMap;

  /// The channels already written to `topics.txt`, used to record each topic only once.
  ChannelIdSet mRecordedTopics;

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

  /// @brief Append @p topic and @p schemaName to the run's `topics.txt`, but only the first time
  /// @p channelId is seen.
  ///
  /// Called by @ref publisher while opening a channel. Subsequent calls for the same @p channelId
  /// are ignored, so a topic is listed exactly once.
  ///
  /// @param channelId Identifies the channel; the first sighting drives whether the topic is
  /// appended.
  ///
  /// @param topic The topic name to append.
  ///
  /// @param schemaName The human-readable name of the channel's payload schema.
  void recordTopic(
      ChannelId channelId,
      const std::string_view& topic,
      const std::string_view& schemaName);
};

} // namespace nioc::terminus
