////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/chronicle/memoryCrate.hpp>

namespace nioc::chronicle
{


/// @brief Memory-mapped backing for a @ref MemoryCrate: a span into a shared mapped data roll.
///
/// Holds a shared memory mapping of a data roll plus the byte range of one frame within it. The
/// frame stays valid as long as any crate keeps the mapping alive, so handing out the bytes costs
/// no copy.
class MemoryCrate::MmapMemoryCrate
{
public:
  using MappedFile = boost::iostreams::mapped_file_source;

  using MappedFilePtr = std::shared_ptr<MappedFile>;

  /// @brief Pins one frame's bytes within a mapped data roll.
  ///
  /// @param mappedFilePtr Shared mapping of the data roll holding the frame.
  ///
  /// @param indexEntry Roll, offset, and size locating the frame within the mapping.
  MmapMemoryCrate(MappedFilePtr mappedFilePtr, const IndexEntry& indexEntry);

  MmapMemoryCrate(const MmapMemoryCrate&) = default;

  MmapMemoryCrate(MmapMemoryCrate&&) = default;

  ~MmapMemoryCrate() = default;

  MmapMemoryCrate& operator=(const MmapMemoryCrate&) = delete;

  MmapMemoryCrate& operator=(MmapMemoryCrate&&) = delete;

  /// @brief Returns a read-only view of the frame's bytes.
  [[nodiscard]] const std::span<const std::byte>& span() const noexcept;

private:
  MappedFilePtr mMappedFilePtr;

  std::span<const std::byte> mSpan;
};


} // namespace nioc::chronicle
