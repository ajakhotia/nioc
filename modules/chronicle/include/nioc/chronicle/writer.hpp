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
/// Appends each frame to its channel in arrival order. Thread-safe: a frame's bytes are never torn
/// or interleaved. The order of writes that race is unspecified, so writes whose order matters must
/// come from one thread.
class Writer
{
public:
  /// @brief Default maximum size of one data file (128 MiB).
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Records into @p rootDir.
  ///
  /// @param rootDir Existing empty directory to write into.
  ///
  /// @param ioMechanism How to write the data.
  ///
  /// @param maxFileSizeInBytes Maximum size of one data file.
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

  /// @brief Appends one frame to a channel.
  ///
  /// @param channelId Channel to append to.
  ///
  /// @param data Frame payload.
  ///
  /// @throws std::invalid_argument On a channel's first frame if the I/O mechanism cannot write.
  void write(ChannelId channelId, const std::span<const std::byte>& data);

  /// @brief Appends several spans to a channel as one frame.
  ///
  /// @param channelId Channel to append to.
  ///
  /// @param data Spans joined into one frame.
  ///
  /// @throws std::invalid_argument On a channel's first frame if the I/O mechanism cannot write.
  void write(ChannelId channelId, std::span<const std::span<const std::byte>> data);

  /// @brief Returns the chronicle directory.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  using ChannelPtrMap = std::unordered_map<ChannelId, std::unique_ptr<ChannelWriter>>;

  const IoMechanism mIoMechanism;
  const std::filesystem::path mLogDirectory;
  const std::size_t mMaxFileSizeInBytes;
  common::Locked<std::ofstream> mLockedSequenceFile;
  common::Locked<ChannelPtrMap> mLockedChannelPtrMap;

  ChannelWriter& acquireChannel(ChannelId channelId, ChannelPtrMap& channelPtrMap);
};

} // namespace nioc::chronicle
