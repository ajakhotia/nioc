////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "defines.hpp"
#include "memoryCrate.hpp"
#include <memory>

namespace naksh::logger
{

class LogEntry
{
public:
    LogEntry(ChannelId channelId, MemoryCrate memoryCrate);

    LogEntry(const LogEntry&) = default;

    LogEntry(LogEntry&&) noexcept = default;

    ~LogEntry() = default;

    LogEntry& operator=(const LogEntry&) = default;

    LogEntry& operator=(LogEntry&&) = default;

    [[nodiscard]] ChannelId channelId() const noexcept;

    [[nodiscard]] std::span<const std::byte> span() const noexcept;

private:
    ChannelId mChannelId;

    MemoryCrate mMemoryCrate;
};


class LogReader
{
public:
    explicit LogReader(std::filesystem::path logRoot);

    LogReader(const LogReader&) = delete;

    LogReader(LogReader&& logReader) noexcept;

    ~LogReader();

    LogReader& operator=(const LogReader&) = delete;

    LogReader& operator=(LogReader&& logReader) noexcept;

    LogEntry read();

private:
    class LogReaderImpl;
    std::unique_ptr<LogReaderImpl> mLogReaderImpl;
};

} // namespace naksh::logger
