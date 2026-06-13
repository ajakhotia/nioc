////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "channelReader.hpp"
#include "defines.hpp"
#include "memoryCrate.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <memory>
#include <nioc/common/locked.hpp>
#include <unordered_map>

namespace nioc::chronicle
{

/// @brief One frame read from a chronicle: its channel ID and its data.
struct Entry
{
  ChannelId mChannelId;
  MemoryCrate mMemoryCrate;
};

/// @brief Reads a chronicle's entries back for playback.
///
/// Returns entries in the order they were written. Not thread-safe: use from one thread only.
class Reader
{
public:
  /// @brief Opens the chronicle in @p logRoot for reading.
  ///
  /// @param logRoot Path to the chronicle directory.
  ///
  /// @param ioMechanism How to read the data.
  ///
  /// @throws std::invalid_argument If @p logRoot does not exist or is not a directory.
  explicit Reader(std::filesystem::path logRoot, IoMechanism ioMechanism = IoMechanism::Mmap);

  Reader(const Reader&) = delete;

  Reader(Reader&&) noexcept = delete;

  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&&) noexcept = delete;

  /// @brief Reads the next entry.
  /// @return The next entry.
  /// @throws std::runtime_error At end of the chronicle.
  /// @throws std::invalid_argument If the I/O mechanism does not support reading.
  Entry read();

private:
  using ChannelReaderMap = std::unordered_map<ChannelId, std::unique_ptr<ChannelReader>>;

  const IoMechanism mIoMechanism;
  const std::filesystem::path mLogRoot;
  const boost::iostreams::mapped_file_source mSequenceFile;
  std::uint64_t mNextReadIndex{0ULL};
  common::Locked<ChannelReaderMap> mLockedChannelReaderMap;

  ChannelReader& acquireChannel(ChannelId channelId, ChannelReaderMap& channelReaderMap);
};

} // namespace nioc::chronicle
