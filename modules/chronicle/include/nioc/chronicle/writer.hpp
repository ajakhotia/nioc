////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "channelWriter.hpp"
#include "defines.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <nioc/common/locked.hpp>
#include <span>
#include <unordered_map>

namespace nioc::chronicle
{

/// @brief Records data frames to a chronicle for later playback.
///
/// Appends frames to their channels in arrival order, preserving the exact sequence of events for
/// replay.
class Writer
{
public:
  /// @brief Default cap on the size of a single data file (128 MiB).
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Constructs a Writer that records into @p rootDir.
  ///
  /// @param rootDir Existing empty directory that this Writer populates.
  ///
  /// @param ioMechanism I/O mechanism used to write data.
  ///
  /// @param maxFileSizeInBytes Maximum size of a single data file.
  ///
  /// @throws std::invalid_argument If @p rootDir does not exist, is not a directory, or is not
  /// empty.
  explicit Writer(
      std::filesystem::path rootDir,
      IoMechanism ioMechanism = IoMechanism::Stream,
      std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  Writer(const Writer&) = delete;

  Writer(Writer&&) noexcept = delete;

  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  /// @brief Appends one data frame to a channel.
  ///
  /// @param channelId Channel the frame belongs to.
  ///
  /// @param data Frame payload.
  ///
  /// @throws std::invalid_argument On the first frame of a channel if the Writer's I/O mechanism
  /// does not support writing.
  void write(ChannelId channelId, const std::span<const std::byte>& data);

  /// @brief Appends several spans to a channel as a single frame.
  ///
  /// @param channelId Channel the frame belongs to.
  ///
  /// @param data Spans concatenated into one frame.
  ///
  /// @throws std::invalid_argument On the first frame of a channel if the Writer's I/O mechanism
  /// does not support writing.
  void write(ChannelId channelId, std::span<const std::span<const std::byte>> data);

  /// @brief Returns the chronicle directory path.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  using ChannelPtrMap = std::unordered_map<ChannelId, std::unique_ptr<ChannelWriter>>;

  ChannelWriter& acquireChannel(ChannelId channelId, ChannelPtrMap& channelPtrMap);

  const IoMechanism mIoMechanism;
  const std::filesystem::path mLogDirectory;
  const std::size_t mMaxFileSizeInBytes;
  common::Locked<std::ofstream> mLockedSequenceFile;
  common::Locked<ChannelPtrMap> mLockedChannelPtrMap;
};

} // namespace nioc::chronicle
