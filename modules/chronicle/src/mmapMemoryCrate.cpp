////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapMemoryCrate.hpp"

namespace nioc::chronicle
{
namespace
{
using MappedFile = MemoryCrate::MmapMemoryCrate::MappedFile;
using ConstByteSpan = std::span<const std::byte>;

ConstByteSpan retrieveSpan(const MappedFile& mappedFile, const IndexEntry& indexEntry)
{
  const auto* dataPtr = std::next(
      mappedFile.begin(),
      static_cast<ssize_t>(indexEntry.mRollPosition));
  return ReadWriteUtil<ConstByteSpan>::read(dataPtr, indexEntry.mDataSize);
}

} // namespace

MemoryCrate::MmapMemoryCrate::MmapMemoryCrate(MappedFilePtr mappedFilePtr, const IndexEntry& index):
    mMappedFilePtr(std::move(mappedFilePtr)),
    mSpan(retrieveSpan(*mMappedFilePtr, index))
{
}

const std::span<const std::byte>& MemoryCrate::MmapMemoryCrate::span() const noexcept
{
  return mSpan;
}


} // namespace nioc::chronicle
