////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <unordered_map>

namespace naksh::logger
{
class Logger
{
public:
    static constexpr auto kDefaultFileSize = 128ULL * 1024ULL * 1024ULL;

    /// @brief Constructor
    /// @param logRoot      Root directory to store the log at. The log is written to a child
    ///                     directory with the local date and time as the directory name.
    ///
    /// @param fileSize     Size of files allocated to store the data.
    explicit Logger(std::filesystem::path logRoot = "/tmp/unnamedNakshLogs",
                    size_t fileSize = kDefaultFileSize);

    Logger(const Logger&) = delete;

    Logger(Logger&&) = delete;

    ~Logger() = default;

    Logger& operator=(const Logger&) = delete;

    Logger& operator=(Logger&&) = delete;

    void write(size_t channelId, size_t bufferLength, const void* bufferPtr);

private:
    const std::filesystem::path mLogDirectory;

    const size_t mFileSize;

    // std::unordered_map<size_t, Channel> mChannelMap;
};

} // namespace naksh::logger
