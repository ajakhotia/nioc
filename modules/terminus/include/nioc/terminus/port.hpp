////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msg.hpp"
#include "msgBase.hpp"
#include <boost/program_options.hpp>
#include <filesystem>
#include <memory>
#include <mutex>
#include <nioc/chronicle/writer.hpp>
#include <nioc/concurrent/asyncProcessor.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/logger/logger.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/sink.h>
#include <string>
#include <unordered_map>
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
  using MsgCallback = std::function<void(ConstMsgBasePtr)>;

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

  /// @brief Resolves a previously added resource to its copy inside the working directory.
  ///
  /// The @p source is the original path passed to @ref addResource. The returned path points
  /// at the flat copy that addResource made under the working directory, so callers read from the
  /// self-contained recording rather than the resource's original location.
  ///
  /// @param source The original path previously passed to @ref addResource.
  ///
  /// @return The path to the resource's copy inside the working directory.
  ///
  /// @throws std::invalid_argument If the @p source was not previously added (see @ref
  /// addResource).
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source);

  /// @brief Resolves a previously added resource to its copy; const overload, same behavior.
  [[nodiscard]] std::filesystem::path acquireResource(const std::filesystem::path& source) const;

  /// @brief Registers a callback to receive every message published on a channel.
  ///
  /// The Port holds @p callbackPtr weakly: the subscription ends on its own once the callback
  /// expires, so the subscriber controls its own lifetime by keeping the pointer alive. Usually
  /// reached through @ref Component::subscribe rather than called directly.
  ///
  /// @param channelId Channel to receive messages from (see @ref makeChannelId).
  ///
  /// @param callbackPtr Callback invoked with each published message; held weakly.
  void subscribe(ChannelId channelId, std::weak_ptr<const MsgCallback> callbackPtr);

  /// @brief Publishes a message on a channel, fanning it out to subscribers and recording it.
  ///
  /// Delivered synchronously on the caller's thread: every live subscriber on @p channelId is
  /// handed the message into its own inbox, and a copy is teed to the chronicle recorder when this
  /// run records. Thread-safe.
  ///
  /// @param channelId Channel to publish on (see @ref makeChannelId).
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  void publish(ChannelId channelId, const ConstMsgBasePtr& msgPtr);

  /// @brief Publishes a typed message on the channel for a topic.
  ///
  /// Resolves the channel from the message type and @p topic — so two topics carrying the same
  /// schema stay distinct — then publishes as @ref publish(ChannelId, ConstMsgBasePtr) does.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    const auto channelId = makeChannelId(Msg<Schema>::kMsgId, topic);
    logger::trace("publish on topic '{}' (channel {})", topic, channelId.mValue);
    publish(channelId, std::move(msgPtr));
  }

private:
  using SubscriptionList = std::vector<std::weak_ptr<const MsgCallback>>;
  using SubscriptionMap = std::unordered_map<ChannelId, SubscriptionList>;

  /// @brief Fans a message out to its live subscribers and tees a copy to the recorder.
  void dispatch(ChannelId channelId, const ConstMsgBasePtr& msgBasePtr);

  const std::filesystem::path mWorkingDir;
  const std::shared_ptr<spdlog::sinks::sink> mConsoleLogSink;
  const nlohmann::json mConfig;
  std::unordered_map<std::string, std::string> mResourceMap;
  const std::unique_ptr<chronicle::Writer> mChronicleWriter;

  std::mutex mSubscriptionMutex;
  SubscriptionMap mSubscriptionMap;

  // The recorder appends teed messages to the chronicle on its own thread; present only when this
  // run records. publish() fans out to subscribers synchronously, then tees a copy here.
  std::shared_ptr<concurrent::AsyncProcessor<std::pair<ChannelId, ConstMsgBasePtr>>> mRecorder;
  std::shared_ptr<concurrent::ThreadedRunner> mRecorderRunner;
};

} // namespace nioc::terminus
