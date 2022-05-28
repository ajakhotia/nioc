////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "logReaderImpl.hpp"

#include "utils.hpp"

namespace naksh::logger
{
namespace fs = std::filesystem;

namespace
{

fs::path validatePath(fs::path logRoot)
{
    if(not fs::exists(logRoot))
    {
        throw std::invalid_argument("[LogReader::LogReader] Directory " + logRoot.string() +
                                    " does not exist.");
    }

    return logRoot;
}

} // namespace


LogReader::LogReaderImpl::LogReaderImpl(std::filesystem::path logRoot):
    mLogRoot(validatePath(std::move(logRoot))),
    mSequenceFile(mLogRoot / kSequenceFileName),
    mNextReadIndex(0ULL)
{
}


LogEntry LogReader::LogReaderImpl::read()
{
    const auto indexPtrOffset = mNextReadIndex * sizeof(SequenceEntry);

    if(indexPtrOffset >= mSequenceFile.size())
    {
        throw std::runtime_error("Reached end of sequence file at " +
                                 (mLogRoot / kSequenceFileName).string());
    }

    ++mNextReadIndex;

    const auto sequenceEntry = ReadWriteUtil<SequenceEntry>::read(
        std::next(mSequenceFile.data(), ssize_t(indexPtrOffset)));

    auto& channelReader = acquireChannel(sequenceEntry.mChannelId);
    return {sequenceEntry.mChannelId, channelReader.read()};
}


ChannelReader& LogReader::LogReaderImpl::acquireChannel(ChannelId channelId)
{
    return mLockedChannelReaderMap(
        [&](ChannelReaderMap& channelReaderMap) -> ChannelReader&
        {
            if(not channelReaderMap.contains(channelId))
            {
                channelReaderMap.try_emplace(channelId,
                                             ChannelReader(mLogRoot / toHexString(channelId)));
            }

            return channelReaderMap.at(channelId);
        });
}


} // namespace naksh::logger
