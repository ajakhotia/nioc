////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "configStore.hpp"
#include "consignment.hpp"
#include "manifest.hpp"
#include "msg.hpp"
#include "msgBase.hpp"
#include "runContext.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <nioc/common/locked.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nioc/concurrent/runner.hpp>
#include <spdlog/fwd.h>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nioc::terminus
{

class AsyncChronicleWriter;
class Component;
class Driver;

/// @brief Central data hub for a nioc binary.
///
/// One Port instance owns the on-disk recording for one run of a nioc binary. It holds the run's
/// @ref Manifest, which records itself into the recording (`manifest.json` and `config.json`) and
/// is exposed via @ref runContext and @ref config. The Port creates the recording directory,
/// tracks the recording's resource copies (`resources.json`, rewritten at teardown), adds a file
/// sink to the nioc default logger so the run's log output is also written into the recording,
/// and opens the chronicle writer that downstream code uses to record time-series data.
///
/// The Port also owns the run's routine graph: the @ref Setup callback passed at construction
/// builds the Drivers and Components bound to this Port, launches them on Runners, and hands all
/// three sets over to the Port. On destruction, the Port requests shutdown, waits for in-flight
/// consignments to drain, destroys the drivers, components, and runners in that order, and then
/// finalizes the recording by writing `metadata.json` (the verbatim launch command plus the
/// resource manifest) and detaching the log-file sink.
class Port
{
public:
  using ChannelId = chronicle::ChannelId;
  using ConsignmentCallback = std::function<void(Consignment)>;

  using Drivers = std::vector<std::shared_ptr<Driver>>;
  using Components = std::vector<std::shared_ptr<Component>>;
  using Runners = std::vector<std::shared_ptr<concurrent::Runner>>;

  /// @brief Callback that builds the run's routine graph.
  ///
  /// Invoked exactly once at the end of construction, with the fully initialized Port and the
  /// three routine sets the Port owns. The callback constructs Drivers and Components bound to the
  /// Port, launches them on Runners, and parks all of them in the supplied vectors; the Port
  /// destroys them at destruction in dependency order (drivers, components, runners).
  using Setup = std::function<void(Port&, Drivers&, Components&, Runners&)>;

  /// @brief Constructs a Port from a run's manifest and creates a fresh recording directory.
  ///
  /// The recording directory is named `<iso8601>_<uuid>` and lives directly under the context's
  /// log root, which is created if it does not exist. The run's log output is also written to
  /// `console.log` inside it; the manifest records itself (`manifest.json`, `config.json`); each
  /// listed resource is copied in and mapped in `resources.json`; and, when the context asks for
  /// it, chronicle data lands in `chronicle/`.
  ///
  /// @param manifest The run's context and configuration (see @ref Manifest); the Port takes
  /// ownership.
  ///
  /// @param setup Builds the run's routine graph (see @ref Setup); invoked once the recording is
  /// set up.
  ///
  /// @throws std::invalid_argument If a listed resource is missing, not a regular file, or
  /// collides.
  ///
  /// @throws std::runtime_error If the recording directory cannot be populated.
  explicit Port(Manifest manifest, const Setup& setup);

  Port(const Port&) = delete;

  Port(Port&&) noexcept = delete;

  ~Port();

  Port& operator=(const Port&) = delete;

  Port& operator=(Port&&) noexcept = delete;

  /// @brief Returns the working directory that holds this run's recording.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// @brief Returns this run's context: its mode, input log, and recording settings.
  ///
  /// Routines receive a Port at construction, so cross-cutting context — say, buffering decisions
  /// that depend on @ref RunContext::playback — flows through here rather than through their
  /// config blocks.
  [[nodiscard]] const RunContext& runContext() const noexcept;

  /// @brief Returns a typed read-only view of the run's configuration.
  ///
  /// The returned reader and its sub-readers stay valid for the Port's lifetime.
  ///
  /// @tparam Schema Compiled type of the root schema the Port's manifest was built against.
  ///
  /// @throws kj::Exception If @p Schema is not that root schema.
  template<typename Schema>
  [[nodiscard]] typename Schema::Reader config() const
  {
    return mManifest.mConfigStore.get<Schema>();
  }

  /// @brief Adds a supporting file to the recording.
  ///
  /// Copies the file at the @p source location into the recording directory under its filename. The
  /// mapping `originalPath → filename` is recorded in `metadata.json` at shutdown.
  ///
  /// @param source Path to the file being added.
  ///
  /// @throws std::invalid_argument If the @p source does not exist, is not a regular file, or its
  /// filename collides with a previously added resource.
  void addResource(const std::filesystem::path& source);

  /// @brief Returns the working-directory copy of a resource, adding it first if needed.
  ///
  /// Looks up the copy of a @p source inside the working directory. If the @p source was not added
  /// yet, the method copies it first (see @ref addResource). Callers then read from the
  /// self-contained recording rather than the resource's original location.
  ///
  /// @param source The resource's original path.
  ///
  /// @return The path to the resource's copy inside the working directory.
  ///
  /// @throws std::invalid_argument If a @p source must be added but is missing, is not a regular
  /// file, or its filename collides with an already-added resource.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source);

  /// @brief Returns the working-directory copy of a previously added resource.
  ///
  /// Const overload. Unlike the non-const overload, it never adds a resource; the p @p source must
  /// already be added (see @ref addResource).
  ///
  /// @param source The original path of a previously added resource.
  ///
  /// @return The path to the resource's copy inside the working directory.
  ///
  /// @throws std::out_of_range If a @p source was not previously added.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

  /// @brief Registers a callback to receive every message published on a channel.
  ///
  /// The Port owns @p callback for the rest of the run: subscriptions are fixed once built, so the
  /// callback is held directly and invoked on the publishing thread with no per-message lifetime
  /// check. Usually reached through @ref Component::subscribe rather than called directly.
  ///
  /// @param channelId Channel to receive messages from (see @ref makeChannelId).
  ///
  /// @param callback Callback invoked with each published message. Multiple callbacks may share a
  /// channel; all of them receive every message.
  void subscribe(ChannelId channelId, ConsignmentCallback callback);

  /// @brief Publishes a typed message on a topic, fanning it out to subscribers and recording it.
  ///
  /// Resolves the channel from @p Schema and @p topic, then delivers synchronously on the caller's
  /// thread: every subscriber on that channel is handed the message into its own inbox, and a copy
  /// is recorded to the chronicle when this run records. Thread-safe.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to publish on; combined with @p Schema into a channel (see @ref
  /// makeChannelId).
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);
    recordTopic(channelId, topic, common::prettyName<Schema>());
    publish(channelId, std::move(msgPtr));
  }

  /// @brief Requests a graceful shutdown: producers stop and in-flight work drains.
  ///
  /// Trips @ref shutdownToken. A @ref Driver winds down off it.
  void shutdown() const noexcept;

  /// @brief Requests an immediate halt: stop now and abandon in-flight work.
  ///
  /// Trips @ref haltToken. A @ref Component finishes off it.
  void abort() const noexcept;

  /// @brief Returns the token tripped by @ref shutdown.
  [[nodiscard]] std::stop_token shutdownToken() const noexcept;

  /// @brief Returns the token tripped by @ref halt.
  [[nodiscard]] std::stop_token abortToken() const noexcept;

  void awaitQuiescence() const;

  /// @brief Paces one beat of the caller's main loop, reporting whether the run is still live.
  ///
  /// Records the invocation time, runs @p housekeeping, and then inspects the drivers: once every
  /// driver reports it is done, the method returns false right away so the caller's loop can end.
  /// Otherwise it sleeps out the remainder of @p duration — measured from invocation, so the
  /// housekeeping cost does not stretch the beat — and returns true.
  ///
  /// @param duration Length of one beat of the loop.
  ///
  /// @param housekeeping Work to run on this beat, on the caller's thread.
  ///
  /// @return True while at least one driver is still working; false once all drivers are done.
  [[nodiscard]] bool wait(
      std::chrono::nanoseconds duration,
      const std::function<void()>& housekeeping) const;

private:
  using ResourceMap = std::unordered_map<std::string, std::string>;
  using SubscriptionList = std::vector<ConsignmentCallback>;
  using SubscriptionMap = std::unordered_map<ChannelId, SubscriptionList>;
  using ChannelIdSet = std::unordered_set<ChannelId>;

  const Manifest mManifest;
  const std::filesystem::path mWorkingDir;
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;

  common::Locked<ResourceMap> mLockedResourceMap;
  common::Locked<SubscriptionMap> mLockedSubscriptionMap;
  common::Locked<ChannelIdSet> mLockedChannelIdSet;

  mutable std::atomic_uint32_t mPendingConsignments{0};
  std::stop_source mShutdownSource;
  std::stop_source mAbortSource;
  std::unique_ptr<AsyncChronicleWriter> mChronicleWriter;

  // The run's routine graph, built by the setup callback. Declared in this order so natural
  // destruction mirrors the destructor's explicit drivers → components → runners teardown.
  Runners mRunners;
  Components mComponents;
  Drivers mDrivers;

  /// @brief Fans a message out to subscribers for a given channel.
  ///
  /// @param channelId Channel to publish on.
  ///
  /// @param msgBasePtr Message to publish; ownership passes to the Port.
  void publish(ChannelId channelId, const ConstMsgBasePtr& msgBasePtr);

  /// @brief Makes note of a topic in human-readable form.
  ///
  /// @param channelId Channel id of the message.
  ///
  /// @param topic Human-readable name of the topic.
  ///
  /// @param schemaName Human-readable name of the message schema published on the topic.
  void recordTopic(
      ChannelId channelId,
      const std::string_view& topic,
      const std::string_view& schemaName);
};

} // namespace nioc::terminus
