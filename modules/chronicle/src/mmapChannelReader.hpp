////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/chronicle/channelReader.hpp>
#include <nioc/chronicle/memoryCrate.hpp>

namespace nioc::chronicle
{

/// @brief Memory-mapped @ref ChannelReader: returns each frame as a view into the mapped data roll.
///
/// Reads one channel directory. It memory-maps the channel's index file and, on each @ref read,
/// resolves the next entry to its data roll, mapping that roll lazily and returning the frame bytes
/// in place — no copy. Recently used rolls are kept mapped in a small buffer.
class MmapChannelReader final: public ChannelReader
{
public:
  using MappedFile = boost::iostreams::mapped_file_source;

  using MappedFilePtr = std::shared_ptr<MappedFile>;

  /// @brief Pairs a data roll's identifier with its memory mapping.
  struct MappedLogRoll
  {
    std::uint64_t mRollId;
    MappedFilePtr mMappedFilePtr;
  };

  /// @brief Opens the channel directory and maps its index file.
  /// @param logRoot Path to the channel directory to read.
  explicit MmapChannelReader(std::filesystem::path logRoot);

  MmapChannelReader(const MmapChannelReader&) = delete;

  MmapChannelReader(MmapChannelReader&&) = delete;

  ~MmapChannelReader() override = default;

  MmapChannelReader& operator=(const MmapChannelReader&) = delete;

  MmapChannelReader& operator=(MmapChannelReader&&) = delete;

  [[nodiscard]] MemoryCrate read() override;

private:
  const std::filesystem::path mLogRoot;

  const MappedFile mIndexFile;

  std::uint64_t mNextReadIndex{0ULL};

  boost::circular_buffer<MappedLogRoll> mLogRollBuffer;

  MappedFilePtr acquireLogRoll(std::uint64_t rollId);
};

} // namespace nioc::chronicle
