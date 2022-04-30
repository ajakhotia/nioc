////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <naksh/logger/channelReader.hpp>
#include <naksh/logger/memoryCrate.hpp>
#include <unordered_map>

namespace naksh::logger
{

class LogEntry
{
public:
    using ChannelId = std::uint64_t;

    LogEntry(ChannelId channelId, MemoryCrate memoryCrate);

    LogEntry(const LogEntry&) = default;

    LogEntry(LogEntry&&) noexcept = default;

    ~LogEntry() = default;

    LogEntry& operator=(const LogEntry&) = default;

    LogEntry& operator=(LogEntry&&) = default;

private:
    ChannelId mChannelId;

    MemoryCrate mMemoryCrate;
};


class LogReader
{
public:
    using ChannelId = std::uint64_t;

    explicit LogReader(std::filesystem::path logRoot);

    LogReader(const LogReader&) = delete;

    LogReader(LogReader&&) noexcept = delete;

    ~LogReader() = default;

    LogReader& operator=(const LogReader&) = delete;

    LogReader& operator=(LogReader&&) noexcept = delete;

    LogEntry read();

private:
    std::filesystem::path mLogRoot;

    std::unordered_map<ChannelId, ChannelReader> mChannelReaderMap;
};

} // namespace naksh::logger
