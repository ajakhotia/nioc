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

/// @brief Writes messages to a chronicle on a dedicated thread.
///
/// Construction opens the chronicle and starts the writer thread; destruction stops it. @ref push
/// returns at once; the disk write runs later on the writer thread, not the calling thread.
class AsyncChronicleWriter
{
public:
  using ChannelId = chronicle::ChannelId;

  /// @brief Opens the chronicle and starts the writer thread.
  ///
  /// @param chronicleDir Directory to write the chronicle into. Created if it does not exist.
  explicit AsyncChronicleWriter(const std::filesystem::path& chronicleDir);

  AsyncChronicleWriter(const AsyncChronicleWriter&) = delete;

  AsyncChronicleWriter(AsyncChronicleWriter&&) noexcept = delete;

  /// @brief Stops the writer thread and joins it.
  ///
  /// Any messages still queued are dropped, not written to disk.
  ~AsyncChronicleWriter() = default;

  AsyncChronicleWriter& operator=(const AsyncChronicleWriter&) = delete;

  AsyncChronicleWriter& operator=(AsyncChronicleWriter&&) noexcept = delete;

  /// @brief Queues a message to write to the chronicle on @p channelId.
  ///
  /// Thread-safe and non-blocking. The disk write happens later on the writer thread.
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
