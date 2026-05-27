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

/// @brief Writes data to a chronicle for later playback.
///
/// Records data to channels in the order it arrives. This preserves the exact
/// sequence of events for replay.
class Writer
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Constructs a Writer that writes into @p rootDir.
  /// @param rootDir            Existing empty directory that this Writer will populate.
  /// @param ioMechanism        I/O mechanism to use for writing data.
  /// @param maxFileSizeInBytes Maximum size of individual data files.
  /// @throws std::invalid_argument If @p rootDir does not exist, is not a directory, or is
  ///                               not empty; or if @p ioMechanism is not supported for
  ///                               writing.
  explicit Writer(
      std::filesystem::path rootDir,
      IoMechanism ioMechanism = IoMechanism::Stream,
      std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  Writer(const Writer&) = delete;

  Writer(Writer&&) noexcept = delete;

  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  /// @brief Writes a data frame to a channel.
  /// @param channelId Channel identifier.
  /// @param data Data to write.
  void write(ChannelId channelId, const std::span<const std::byte>& data);

  /// @brief Writes multiple data spans as a single frame to a channel.
  /// @param channelId Channel identifier.
  /// @param data Data spans to write as one frame.
  void write(ChannelId channelId, std::span<const std::span<const std::byte>> data);

  /// @brief Gets the chronicle directory path.
  /// @return Path to the chronicle directory.
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
