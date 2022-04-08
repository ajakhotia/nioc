////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "channelReaderImpl.hpp"


namespace naksh::logger
{
namespace
{
constexpr const auto kLogRollBufferSize = 5UL;

}

ChannelReader::ChannelReaderImpl::ChannelReaderImpl(std::filesystem::path logRoot):
    mLogRoot(std::move(logRoot)), mLogRollBuffer(kLogRollBufferSize)
{
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
