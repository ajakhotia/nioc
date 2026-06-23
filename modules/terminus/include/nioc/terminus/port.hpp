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

/// @brief Central data hub for one run of a nioc binary.
///
/// A Port owns the on-disk recording for one run: it creates the recording directory, holds the
/// run's @ref Manifest (exposed via @ref runContext and @ref config), copies in supporting files,
/// writes the run's log output into the recording, and opens the chronicle that backs every
/// message.
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
  /// The recording directory is named `<iso8601>_<uuid>`, under the context's log root. Inside it:
  /// log output goes to `console.log`; the manifest writes `manifest.json` and `config.json`; each
  /// listed resource is copied in and mapped in `resources.json`; chronicle data goes to
  /// `chronicle/`.
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
  /// @param source The resource's original path.
  ///
  /// @return Path to the resource's copy inside the working directory.
  ///
  /// @throws std::invalid_argument If @p source must be added but is missing, is not a regular
  /// file, or its filename collides with an already-added resource.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source);

  /// @brief Returns the working-directory copy of an already-added resource.
  ///
  /// @param source The original path of an already-added resource.
  ///
  /// @return Path to the resource's copy inside the working directory.
  ///
  /// @throws std::out_of_range If @p source was not added.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

  /// @brief Mints a producer handle for a topic.
  ///
  /// Call once at construction and keep the handle. The same @p Schema on two topics yields two
  /// channels.
  ///
  /// @tparam Schema Cap'n Proto schema of the messages.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @return A @ref Publisher for the topic.
  ///
  /// @throws std::logic_error If the run does not record; a publisher needs a channel to record and
  /// reserve into (non-recording publish is not supported for now).
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

  /// @brief Registers a callback to receive every message delivered on a channel.
  ///
  /// Call only at construction; the communication graph is frozen once the run starts. Usually
  /// reached through @ref Component::subscribe.
  ///
  /// @param channelId Channel to receive frames from.
  ///
  /// @param callback Callback run with each delivered frame. Multiple callbacks may share a
  /// channel; all of them receive every frame.
  void subscribe(ChannelId channelId, ConsignmentCallback callback);

  /// @brief Delivers a frame to a channel's subscribers.
  ///
  /// The delivery primitive behind both a fresh @ref Publisher::publish and a @ref LogPlayer
  /// replay: the frame is handed to every callback subscribed to @p channelId. A channel with no
  /// subscribers drops the frame.
  ///
  /// @param channelId Channel the frame belongs to.
  ///
  /// @param crate Frame to deliver; copied to each subscriber.
  void deliver(ChannelId channelId, const chronicle::Crate& crate) const;

  /// @brief Requests a graceful shutdown: producers stop and in-flight work drains.
  void shutdown() const noexcept;

  /// @brief Requests an immediate halt: stop now and abandon in-flight work.
  void abort() const noexcept;

  /// @brief Returns the token tripped by @ref shutdown.
  [[nodiscard]] std::stop_token shutdownToken() const noexcept;

  /// @brief Returns the token tripped by @ref abort.
  [[nodiscard]] std::stop_token abortToken() const noexcept;

  /// @brief Blocks until no delivered frame is in flight.
  ///
  /// Call after producers have stopped, to let in-flight work drain before tearing down the routine
  /// graph. A subscriber that holds onto its frames forever prevents this call from returning.
  void awaitQuiescence() const;

  /// @brief Paces one beat of the caller's main loop and reports whether the run is still live.
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
  const std::unique_ptr<chronicle::Writer> mWriter;

  common::Locked<ResourceMap> mLockedResourceMap;
  ChannelIdSet mRecordedTopics;
  SubscriptionMap mSubscriptionMap;

  mutable std::atomic_uint32_t mPendingConsignments{0};
  std::stop_source mShutdownSource;
  std::stop_source mAbortSource;

  // The run's routine graph. Declared in this order, so natural member destruction runs drivers →
  // components → runners, matching the destructor's teardown order.
  Runners mRunners;
  Components mComponents;
  Drivers mDrivers;

  /// @brief Records a topic in human-readable form, once per channel.
  void recordTopic(
      ChannelId channelId,
      const std::string_view& topic,
      const std::string_view& schemaName);
};

} // namespace nioc::terminus
