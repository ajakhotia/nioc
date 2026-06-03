////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"
#include <filesystem>
#include <memory>
#include <nioc/chronicle/writer.hpp>
#include <nioc/concurrent/asyncProcessor.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <utility>

namespace nioc::terminus
{

/// @brief Writes published messages to a chronicle on a dedicated thread.
///
/// Owns the chronicle writer, the queue that buffers messages handed to @ref record, and the thread
/// that drains it. Construction opens the chronicle under the given directory and launches the
/// writer thread; destruction stops that thread. @ref record returns immediately — the on-disk
/// write happens on the writer's own thread, off the publishing thread.
class AsyncChronicleWriter
{
public:
  using ChannelId = chronicle::ChannelId;

  /// @brief Opens the chronicle under @p chronicleDir and launches the writer thread.
  ///
  /// @param chronicleDir Directory the chronicle is written into; created if it does not exist.
  explicit AsyncChronicleWriter(const std::filesystem::path& chronicleDir);

  AsyncChronicleWriter(const AsyncChronicleWriter&) = delete;

  AsyncChronicleWriter(AsyncChronicleWriter&&) noexcept = delete;

  /// @brief Stops the writer thread, joining it before returning.
  ///
  /// Stopping is abrupt: messages still queued are dropped, which a planned drain-on-close pass
  /// (shared with Component) will address.
  ~AsyncChronicleWriter();

  AsyncChronicleWriter& operator=(const AsyncChronicleWriter&) = delete;

  AsyncChronicleWriter& operator=(AsyncChronicleWriter&&) noexcept = delete;

  /// @brief Queues a message to be written to the chronicle on @p channelId.
  ///
  /// Thread-safe and non-blocking: the value is enqueued and the on-disk write happens later on the
  /// writer's own thread.
  ///
  /// @param channelId Channel the message was published on.
  ///
  /// @param msgPtr Message to record.
  void push(ChannelId channelId, const ConstMsgBasePtr& msgPtr);

private:
  using Processor = concurrent::AsyncProcessor<std::pair<ChannelId, ConstMsgBasePtr>>;

  chronicle::Writer mWriter;
  std::shared_ptr<Processor> mProcessor;
  std::shared_ptr<concurrent::ThreadedRunner> mRunner;

  Processor& processor();

  const Processor& processor() const;
};

} // namespace nioc::terminus
