////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"

#include <boost/circular_buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <naksh/logger/channelReader.hpp>

namespace naksh::logger
{

class ChannelReader::ChannelReaderImpl
{
public:
    using MappedFile = boost::iostreams::mapped_file_source;

    using MappedFilePtr = std::shared_ptr<MappedFile>;

    struct MappedLogRoll
    {
        std::uint64_t mRollId;
        MappedFilePtr mMappedFilePtr;
    };

    explicit ChannelReaderImpl(std::filesystem::path logRoot);

    ChannelReaderImpl(const ChannelReaderImpl&) = delete;

    ChannelReaderImpl(ChannelReaderImpl&&) = delete;

    ~ChannelReaderImpl() = default;

    ChannelReaderImpl& operator=(const ChannelReaderImpl&) = delete;

    ChannelReaderImpl& operator=(ChannelReaderImpl&&) = delete;


private:
    std::filesystem::path mLogRoot;

    boost::circular_buffer<MappedLogRoll> mLogRollBuffer;

    MappedFilePtr acquireLogRoll(std::uint64_t rollId);
};

} // namespace naksh::logger
