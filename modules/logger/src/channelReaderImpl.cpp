////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "channelReaderImpl.hpp"
#include "memoryCrateImpl.hpp"

namespace naksh::logger
{
namespace fs = std::filesystem;

namespace
{
constexpr const auto kLogRollBufferSize = 5UL;

fs::path validatePath(fs::path logRoot)
{
    if(not fs::exists(logRoot))
    {
        throw std::invalid_argument("[ChannelReader::ChannelReader] Directory " + logRoot.string() +
                                    " does not exist.");
    }

    return logRoot;
}

} // namespace


ChannelReader::ChannelReaderImpl::ChannelReaderImpl(std::filesystem::path logRoot):
    mLogRoot(validatePath(std::move(logRoot))),
    mIndexFile(mLogRoot / kIndexFileName),
    mNextReadIndex(0ULL),
    mLogRollBuffer(kLogRollBufferSize)
{
}


MemoryCrate ChannelReader::ChannelReaderImpl::read()
{
    const auto indexPtrOffset = mNextReadIndex * sizeof(IndexEntry);

    if(indexPtrOffset >= mIndexFile.size())
    {
        throw std::runtime_error("Reached end of index file at " +
                                 (mLogRoot / kIndexFileName).string());
    }

    ++mNextReadIndex;

    const auto index =
        ReadWriteUtil<IndexEntry>::read(std::next(mIndexFile.begin(), ssize_t(indexPtrOffset)));

    auto logRollPtr = acquireLogRoll(index.mRollId);

    return MemoryCrate{
        std::make_shared<MemoryCrate::MemoryCrateImpl>(std::move(logRollPtr), index)};
}


ChannelReader::ChannelReaderImpl::MappedFilePtr
ChannelReader::ChannelReaderImpl::acquireLogRoll(const std::uint64_t rollId)
{
    const auto iter = std::find_if(mLogRollBuffer.begin(),
                                   mLogRollBuffer.end(),
                                   [rollId](const MappedLogRoll& mappedLogRoll)
                                   { return mappedLogRoll.mRollId == rollId; });

    // If the roll doesn't exist, then map it in.
    if(iter == mLogRollBuffer.end())
    {
        auto mappedFilePtr = std::make_shared<MappedFile>(mLogRoot / buildRollName(rollId));
        mLogRollBuffer.push_back({rollId, mappedFilePtr});
        return mappedFilePtr;
    }

    return iter->mMappedFilePtr;
}

} // namespace naksh::logger
