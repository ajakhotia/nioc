////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <fstream>
#include <nioc/chronicle/channelWriter.hpp>
#include <span>

namespace nioc::chronicle
{

/// @brief Stream-based @ref ChannelWriter: appends frames to a channel, rolling data files by size.
///
/// Writes one channel directory. Each frame is appended to the active data roll and indexed; when a
/// frame would push the active roll past its size cap, the writer starts a new roll. This is the
/// write counterpart to @ref MmapChannelReader.
class StreamChannelWriter final: public ChannelWriter
{
public:
  using ConstByteSpan = std::span<const std::byte>;

  /// @brief Default cap on the size of a single data roll (128 MiB).
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Creates the channel directory and its index file.
  ///
  /// @param logRoot Path to the channel directory to create. Must not exist yet.
  ///
  /// @param maxFileSizeInBytes Size cap that triggers a roll to a new data file.
  ///
  /// @throws std::logic_error if @p logRoot already exists.
  /// @throws std::runtime_error if the directory cannot be created.
  explicit StreamChannelWriter(
      std::filesystem::path logRoot,
      std::uint64_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  StreamChannelWriter(const StreamChannelWriter&) = delete;

  StreamChannelWriter(StreamChannelWriter&&) noexcept = delete;

  ~StreamChannelWriter() override = default;

  StreamChannelWriter& operator=(const StreamChannelWriter&) = delete;

  StreamChannelWriter& operator=(StreamChannelWriter&&) noexcept = delete;

  void writeFrame(const ConstByteSpan& data) override;

  void writeFrame(std::span<const ConstByteSpan> dataCollection) override;

private:
  const std::filesystem::path mLogRoot;

  const std::uint64_t mMaxFileSizeInBytes;

  std::ofstream mIndexFile;

  std::uint64_t mRollCounter;

  std::ofstream mActiveLogRoll;

  void rollCheckAndIndex(std::uint64_t requiredSizeInBytes);

  std::filesystem::path nextRollFilePath();
};

} // namespace nioc::chronicle
