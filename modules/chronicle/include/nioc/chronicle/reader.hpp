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

/// @brief A single entry from a chronicle.
///
/// Contains the channel ID and data for one frame.
struct Entry
{
  ChannelId mChannelId;
  MemoryCrate mMemoryCrate;
};

/// @brief Reads data from a chronicle for playback.
///
/// Reads entries in the same order they were written.
class Reader
{
public:
  /// @brief Constructs a Reader.
  /// @param logRoot Path to the chronicle directory.
  /// @param ioMechanism I/O mechanism to use for reading data.
  /// @throws std::invalid_argument If ioMechanism is not supported for reading.
  explicit Reader(std::filesystem::path logRoot, IoMechanism ioMechanism = IoMechanism::Mmap);

  Reader(const Reader&) = delete;

  Reader(Reader&&) noexcept = delete;

  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&&) noexcept = delete;

  /// @brief Reads the next entry.
  /// @return Entry with channel ID and data.
  /// @throws std::runtime_error When end of chronicle is reached.
  Entry read();

private:
  using ChannelReaderMap = std::unordered_map<ChannelId, std::unique_ptr<ChannelReader>>;

  ChannelReader& acquireChannel(ChannelId channelId, ChannelReaderMap& channelReaderMap);

  const IoMechanism mIoMechanism;
  const std::filesystem::path mLogRoot;
  const boost::iostreams::mapped_file_source mSequenceFile;
  std::uint64_t mNextReadIndex{ 0ULL };
  common::Locked<ChannelReaderMap> mLockedChannelReaderMap;
};

} // namespace nioc::chronicle
