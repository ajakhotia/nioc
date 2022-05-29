////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "channel.hpp"
#include <naksh/common/locked.hpp>
#include <naksh/logger/logger.hpp>
#include <unordered_map>

namespace naksh::logger
{

class Logger::LoggerImpl
{
public:
    using ChannelId = std::uint64_t;

    using LockedChannel = common::Locked<Channel>;

    using ChannelPtrMap = std::unordered_map<ChannelId, std::unique_ptr<LockedChannel>>;

    /// @brief Constructor
    /// @param logRoot      Root directory to store the log at. The log is written to a child
    ///                     directory with the local date and time as the directory name.
    ///
    /// @param fileSize     Size of files allocated to store the data.
    explicit LoggerImpl(std::filesystem::path logRoot = kDefaultLogPath,
                        size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

    LoggerImpl(const LoggerImpl&) = delete;

    LoggerImpl(LoggerImpl&&) noexcept = delete;

    ~LoggerImpl() = default;

    LoggerImpl& operator=(const LoggerImpl&) = delete;

    LoggerImpl& operator=(LoggerImpl&&) noexcept = delete;

    void write(size_t channelId, const std::span<const std::byte>& data);

    void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

    const std::filesystem::path& path() const noexcept;

private:
    const std::filesystem::path mLogDirectory;

    const size_t mMaxFileSizeInBytes;

    common::Locked<std::ofstream> mLockedSequenceFile;

    common::Locked<ChannelPtrMap> mLockedChannelPtrMap;

    LockedChannel& acquireChannel(ChannelId channelId);
};

} // namespace naksh::logger
