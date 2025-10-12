////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/chronicle/memoryCrate.hpp>

namespace nioc::chronicle
{

class MmapChannelReader
{
public:
  using MappedFile = boost::iostreams::mapped_file_source;

  using MappedFilePtr = std::shared_ptr<MappedFile>;

  struct MappedLogRoll
  {
    std::uint64_t mRollId;
    MappedFilePtr mMappedFilePtr;
  };

  explicit MmapChannelReader(std::filesystem::path logRoot);

  MmapChannelReader(const MmapChannelReader&) = delete;

  MmapChannelReader(MmapChannelReader&&) = default;

  ~MmapChannelReader() = default;

  MmapChannelReader& operator=(const MmapChannelReader&) = delete;

  MmapChannelReader& operator=(MmapChannelReader&&) = default;

  [[nodiscard]] MemoryCrate read();

private:
  std::filesystem::path mLogRoot;

  MappedFile mIndexFile;

  std::uint64_t mNextReadIndex{ 0ULL };

  boost::circular_buffer<MappedLogRoll> mLogRollBuffer;

  MappedFilePtr acquireLogRoll(std::uint64_t rollId);
};

} // namespace nioc::chronicle
