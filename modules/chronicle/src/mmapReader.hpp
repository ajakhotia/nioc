////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "mmapChannelReader.hpp"
#include <nioc/chronicle/chronicle.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/common/locked.hpp>
#include <unordered_map>

namespace nioc::chronicle
{

class Reader::MmapReader
{
public:
  explicit MmapReader(std::filesystem::path logRoot);

  MmapReader(const MmapReader&) = delete;

  MmapReader(MmapReader&&) = delete;

  ~MmapReader() = default;

  MmapReader& operator=(const MmapReader&) = delete;

  MmapReader& operator=(MmapReader&&) = delete;

  Entry read();

private:
  using ChannelReaderMap = std::unordered_map<ChannelId, MmapChannelReader>;

  std::filesystem::path mLogRoot;

  boost::iostreams::mapped_file_source mSequenceFile;

  std::uint64_t mNextReadIndex{ 0ULL };

  common::Locked<ChannelReaderMap> mLockedChannelReaderMap;

  MmapChannelReader& acquireChannel(ChannelId channelId);
};


} // namespace nioc::chronicle
