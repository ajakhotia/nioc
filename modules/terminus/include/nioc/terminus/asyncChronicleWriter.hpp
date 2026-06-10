////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "consignment.hpp"
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
/// Owns the chronicle writer, the queue that buffers messages handed to @ref push, and the thread
/// that drains it. Construction opens the chronicle under the given directory and launches the
/// writer thread; destruction stops that thread. @ref push returns immediately — the on-disk
/// writing happens on the writer's own thread, off the publishing thread.
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
  /// Messages still queued at this point are dropped; they are not flushed to disk.
  ~AsyncChronicleWriter() = default;

  AsyncChronicleWriter& operator=(const AsyncChronicleWriter&) = delete;

  AsyncChronicleWriter& operator=(AsyncChronicleWriter&&) noexcept = delete;

  /// @brief Queues a message to be written to the chronicle on @p channelId.
  ///
  /// Thread-safe and non-blocking: the value is enqueued and the on-disk writing happens later on
  /// the writer's own thread.
  ///
  /// @param channelId Channel the message was published on.
  ///
  /// @param consignment Message to record.
  void push(ChannelId channelId, Consignment consignment);

private:
  using Processor = concurrent::AsyncProcessor<std::pair<ChannelId, Consignment>>;

  chronicle::Writer mWriter;
  std::shared_ptr<Processor> mProcessor;
  std::shared_ptr<concurrent::ThreadedRunner> mRunner;

  Processor& processor();

  const Processor& processor() const;
};

} // namespace nioc::terminus
