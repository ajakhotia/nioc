////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "asyncChronicleWriter.hpp"
#include "consignment.hpp"
#include "msg.hpp"
#include "msgBase.hpp"
#include <atomic>
#include <boost/program_options.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <nioc/common/locked.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/sink.h>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nioc::terminus
{

/// @brief Central data hub for a nioc binary.
///
/// One Port instance owns the on-disk recording for one run of a nioc binary. It creates the
/// recording directory, captures the launch command, loads and merges the JSON configuration,
/// and adds a file sink to the nioc default logger so the run's log output is also written into the
/// recording. It opens the chronicle writer that downstream code uses to record time-series data.
/// On destruction, it detaches that file sink and finalizes the recording by writing
/// `metadata.json` (the verbatim launch command plus the resource manifest).
class Port
{
public:
  using ChannelId = chronicle::ChannelId;
  using ConsignmentCallback = std::function<void(Consignment)>;

  /// @brief Constructs a Port from a parsed command line.
  ///
  /// Reads Port's own options and the `"commandLine"` entry (see @ref programOptions and
  /// @ref parseCommandLine) out of @p variableMap.
  ///
  /// @param variableMap Parsed options; must come from @ref parseCommandLine, which adds the
  /// `"commandLine"` entry this constructor reads.
  ///
  /// @throws std::invalid_argument If a listed resource is missing, not a regular file, or
  /// collides.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  explicit Port(const boost::program_options::variables_map& variableMap);

  /// @brief Constructs a Port and creates a fresh recording directory.
  ///
  /// The recording directory is named `<iso8601>_<uuid>` and lives directly under @p logRoot,
  /// which is created if it does not exist. The run's log output is also written to `console.log`
  /// inside it; the merged configuration is written to `config.json`; each resource is copied in;
  /// and, when @p writeChronicle is true, chronicle data lands in `chronicle/`.
  ///
  /// @param logRoot Directory under which the fresh recording is created. Created if missing.
  ///
  /// @param configPaths JSON config files merged left-to-right; later entries override earlier
  /// ones.
  ///
  /// @param resourcePaths Files copied into the recording as logged resources (see @ref
  /// addResource).
  ///
  /// @param writeChronicle When true, record the chronicle time-series data stream under
  /// `chronicle/`.
  ///
  /// @param commandLine The verbatim launch command, recorded in `metadata.json`.
  ///
  /// @throws std::invalid_argument If a resource is missing, not a regular file, or collides.
  ///
  /// @throws std::runtime_error If a config file cannot be opened.
  ///
  /// @throws nlohmann::json::parse_error If a config file contains malformed JSON.
  explicit Port(
      const std::filesystem::path& logRoot = std::filesystem::temp_directory_path() / "niocLogs",
      const std::vector<std::filesystem::path>& configPaths = {},
      const std::vector<std::filesystem::path>& resourcePaths = {},
      bool writeChronicle = true,
      const std::string& commandLine = "");

  Port(const Port&) = delete;

  Port(Port&&) noexcept = delete;

  ~Port();

  Port& operator=(const Port&) = delete;

  Port& operator=(Port&&) noexcept = delete;

  /// @brief Returns the working directory that holds this run's recording.
  [[nodiscard]] const std::filesystem::path& workingDir() const noexcept;

  /// @brief Returns the merged JSON configuration.
  [[nodiscard]] const nlohmann::json& config() const noexcept;

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

private:
  using ResourceMap = std::unordered_map<std::string, std::string>;
  using SubscriptionList = std::vector<ConsignmentCallback>;
  using SubscriptionMap = std::unordered_map<ChannelId, SubscriptionList>;
  using ChannelIdSet = std::unordered_set<ChannelId>;

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

  const std::filesystem::path mWorkingDir;
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;
  const nlohmann::json mConfig;

  common::Locked<ResourceMap> mLockedResourceMap;
  common::Locked<SubscriptionMap> mLockedSubscriptionMap;
  common::Locked<ChannelIdSet> mLockedChannelIdSet;

  mutable std::atomic_uint32_t mPendingConsignments{0};
  std::stop_source mShutdownSource;
  std::stop_source mAbortSource;
  std::unique_ptr<AsyncChronicleWriter> mChronicleWriter;
};

} // namespace nioc::terminus
