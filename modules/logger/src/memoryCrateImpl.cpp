////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "memoryCrateImpl.hpp"

namespace naksh::logger
{
namespace
{
using MappedFile = MemoryCrate::MemoryCrateImpl::MappedFile;
using ConstByteSpan = std::span<const std::byte>;

ConstByteSpan retrieveSpan(const MappedFile& mappedFile, const IndexEntry& indexEntry)
{
    const auto* dataPtr = std::next(mappedFile.begin(), ssize_t(indexEntry.mRollPosition));
    return ReadWriteUtil<ConstByteSpan>::read(dataPtr, indexEntry.mDataSize);
}

}


MemoryCrate::MemoryCrateImpl::MemoryCrateImpl(MappedFilePtr mappedFilePtr, const IndexEntry& index):
    mMappedFilePtr(std::move(mappedFilePtr)),
    mSpan(retrieveSpan(*mMappedFilePtr, index))
{
}


const std::span<const std::byte>& MemoryCrate::MemoryCrateImpl::span() const noexcept
{
    return mSpan;
}


} // namespace naksh::logger
