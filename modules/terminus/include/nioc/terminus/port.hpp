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

/// @brief Central data hub for one run of a nioc binary.
///
/// A Port owns the on-disk recording for one run. It creates the recording directory, holds the
/// run's @ref Manifest (exposed via @ref runContext and @ref config), copies in supporting files,
/// writes the run's log output into the recording, and opens the chronicle writer for time-series
/// data.
///
/// A Port also owns the run's routine graph. The @ref Setup callback, passed at construction,
/// builds the Drivers and Components and launches them on Runners. On destruction the Port shuts
/// down, drains in-flight messages, and destroys drivers, components, and runners in that order.
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
  /// Called once at the end of construction with the ready Port and its three routine sets. Build
  /// Drivers and Components bound to the Port, launch them on Runners, and put all of them in the
  /// supplied vectors. The Port destroys them at destruction in this order: drivers, components,
  /// runners.
  using Setup = std::function<void(Port&, Drivers&, Components&, Runners&)>;

  /// @brief Creates a fresh recording directory and builds the run.
  ///
  /// The recording directory is named `<iso8601>_<uuid>`, under the context's log root (created if
  /// missing). Inside it: log output goes to `console.log`; the manifest writes `manifest.json` and
  /// `config.json`; each listed resource is copied in and mapped in `resources.json`; chronicle
  /// data goes to `chronicle/` when the context asks for it.
  ///
  /// @param manifest The run's context and configuration (see @ref Manifest); the Port takes
  /// ownership.
  ///
  /// @param setup Builds the run's routine graph (see @ref Setup); called once the recording is
  /// ready.
  ///
  /// @throws std::invalid_argument If a listed resource is missing, is not a regular file, or its
  /// filename collides.
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
  [[nodiscard]] const RunContext& runContext() const noexcept;

  /// @brief Returns a typed read-only view of the run's configuration.
  ///
  /// The returned reader and its sub-readers stay valid for the Port's lifetime.
  ///
  /// @tparam Schema The root schema the manifest was built against.
  ///
  /// @throws kj::Exception If @p Schema is not that root schema.
  template<typename Schema>
  [[nodiscard]] typename Schema::Reader config() const
  {
    return mManifest.mConfigStore.get<Schema>();
  }

  /// @brief Adds a supporting file to the recording.
  ///
  /// Copies @p source into the recording directory under its filename. The mapping
  /// `originalPath → filename` is recorded in `resources.json` at shutdown.
  ///
  /// @param source Path to the file to add.
  ///
  /// @throws std::invalid_argument If @p source does not exist, is not a regular file, or its
  /// filename collides with an already-added resource.
  void addResource(const std::filesystem::path& source);

  /// @brief Returns the working-directory copy of a resource, adding it first if needed.
  ///
  /// If @p source was not added yet, copies it first (see @ref addResource). Read from the returned
  /// copy so the recording is self-contained.
  ///
  /// @param source The resource's original path.
  ///
  /// @return Path to the resource's copy inside the working directory.
  ///
  /// @throws std::invalid_argument If @p source must be added but is missing, is not a regular
  /// file, or its filename collides with an already-added resource.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source);

  /// @brief Returns the working-directory copy of an already-added resource.
  ///
  /// Const overload. Never adds a resource; @p source must already be added (see @ref addResource).
  ///
  /// @param source The original path of an already-added resource.
  ///
  /// @return Path to the resource's copy inside the working directory.
  ///
  /// @throws std::out_of_range If @p source was not added.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

  /// @brief Registers a callback to receive every message published on a channel.
  ///
  /// The Port owns @p callback for the rest of the run and runs it on the publishing thread.
  /// Usually reached through @ref Component::subscribe rather than called directly.
  ///
  /// @param channelId Channel to receive messages from (see @ref makeChannelId).
  ///
  /// @param callback Callback run with each published message. Multiple callbacks may share a
  /// channel; all of them receive every message.
  void subscribe(ChannelId channelId, ConsignmentCallback callback);

  /// @brief Publishes a typed message on a topic, sending it to subscribers and recording it.
  ///
  /// Delivers synchronously on the caller's thread: every subscriber on the channel gets the
  /// message in its own inbox, and a copy is recorded to the chronicle when this run records.
  /// Thread-safe.
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
  /// Trips @ref shutdownToken, which a @ref Driver watches.
  void shutdown() const noexcept;

  /// @brief Requests an immediate halt: stop now and abandon in-flight work.
  ///
  /// Trips @ref abortToken, which a @ref Component watches.
  void abort() const noexcept;

  /// @brief Returns the token tripped by @ref shutdown.
  [[nodiscard]] std::stop_token shutdownToken() const noexcept;

  /// @brief Returns the token tripped by @ref abort.
  [[nodiscard]] std::stop_token abortToken() const noexcept;

  /// @brief Blocks until no published message is in flight: every message handed to a subscriber
  /// has been destroyed.
  ///
  /// Call after producers have stopped — usually once @ref shutdown has tripped and the drivers are
  /// done — to let in-flight work drain before tearing down the routine graph. A subscriber that
  /// holds onto its messages forever prevents this call from ever returning.
  void awaitQuiescence() const;

  /// @brief Paces one beat of the caller's main loop and reports whether the run is still live.
  ///
  /// Runs @p housekeeping, then checks the drivers. If all drivers are done, returns false at once
  /// so the caller's loop can end. Otherwise sleeps the rest of @p duration — timed from the start
  /// of the call, so housekeeping does not stretch the beat — and returns true.
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

  // The run's routine graph. Declared in this order so natural member destruction runs drivers →
  // components → runners, matching the destructor's teardown order.
  Runners mRunners;
  Components mComponents;
  Drivers mDrivers;

  /// @brief Sends a message to the subscribers of a channel.
  ///
  /// @param channelId Channel to publish on.
  ///
  /// @param msgBasePtr Message to publish; ownership passes to the Port.
  void publish(ChannelId channelId, const ConstMsgBasePtr& msgBasePtr);

  /// @brief Records a topic in human-readable form.
  ///
  /// @param channelId Channel id of the message.
  ///
  /// @param topic Human-readable name of the topic.
  ///
  /// @param schemaName Human-readable name of the message schema on the topic.
  void recordTopic(
      ChannelId channelId,
      const std::string_view& topic,
      const std::string_view& schemaName);
};

} // namespace nioc::terminus
