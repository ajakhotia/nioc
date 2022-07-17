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
    using LockedChannel = common::Locked<Channel>;

    using ChannelPtrMap = std::unordered_map<ChannelId, std::unique_ptr<LockedChannel>>;

    explicit LoggerImpl(std::filesystem::path logRoot, std::size_t maxFileSizeInBytes);

    LoggerImpl(const LoggerImpl&) = delete;

    LoggerImpl(LoggerImpl&&) noexcept = delete;

    ~LoggerImpl() = default;

    LoggerImpl& operator=(const LoggerImpl&) = delete;

    LoggerImpl& operator=(LoggerImpl&&) noexcept = delete;

    void write(ChannelId channelId, const std::span<const std::byte>& data);

    void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

    [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
    const std::filesystem::path mLogDirectory;

    const std::size_t mMaxFileSizeInBytes;

    common::Locked<std::ofstream> mLockedSequenceFile;

    common::Locked<ChannelPtrMap> mLockedChannelPtrMap;

    LockedChannel& acquireChannel(ChannelId channelId);
};

} // namespace naksh::logger
