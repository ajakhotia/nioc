////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/logger/memoryCrate.hpp>

namespace nioc::logger
{

class ChannelReader
{
public:
    using MappedFile = boost::iostreams::mapped_file_source;

    using MappedFilePtr = std::shared_ptr<MappedFile>;

    struct MappedLogRoll
    {
        std::uint64_t mRollId;
        MappedFilePtr mMappedFilePtr;
    };

    explicit ChannelReader(std::filesystem::path logRoot);

    ChannelReader(const ChannelReader&) = delete;

    ChannelReader(ChannelReader&&) = default;

    ~ChannelReader() = default;

    ChannelReader& operator=(const ChannelReader&) = delete;

    ChannelReader& operator=(ChannelReader&&) = default;

    [[nodiscard]] MemoryCrate read();

private:
    std::filesystem::path mLogRoot;

    MappedFile mIndexFile;

    std::uint64_t mNextReadIndex;

    boost::circular_buffer<MappedLogRoll> mLogRollBuffer;

    MappedFilePtr acquireLogRoll(std::uint64_t rollId);
};

} // namespace nioc::logger
