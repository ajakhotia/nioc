////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "defines.hpp"
#include "memoryCrate.hpp"
#include <memory>

namespace nioc::logger
{

struct LogEntry
{
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

} // namespace nioc::logger
