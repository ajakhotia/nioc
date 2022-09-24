////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "channelReader.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/common/locked.hpp>
#include <nioc/logger/logReader.hpp>
#include <unordered_map>


namespace nioc::logger
{

class LogReader::LogReaderImpl
{
public:
    explicit LogReaderImpl(std::filesystem::path logRoot);

    LogReaderImpl(const LogReaderImpl&) = delete;

    LogReaderImpl(LogReaderImpl&&) = delete;

    ~LogReaderImpl() = default;

    LogReaderImpl& operator=(const LogReaderImpl&) = delete;

    LogReaderImpl& operator=(LogReaderImpl&&) = delete;

    LogEntry read();

private:
    using ChannelReaderMap = std::unordered_map<ChannelId, ChannelReader>;

    std::filesystem::path mLogRoot;

    boost::iostreams::mapped_file_source mSequenceFile;

    std::uint64_t mNextReadIndex;

    common::Locked<ChannelReaderMap> mLockedChannelReaderMap;

    ChannelReader& acquireChannel(ChannelId channelId);
};


} // namespace nioc::logger
