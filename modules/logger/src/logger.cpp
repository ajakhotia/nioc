////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

#include <naksh/logger/logger.hpp>
#include <spdlog/spdlog.h>

namespace naksh::logger
{
namespace
{
namespace fs = std::filesystem;

fs::path checkAndSetupLogDirectory(fs::path logRoot)
{
    logRoot /= (timeAsFormattedString(std::chrono::system_clock::now()));

    if(fs::exists(logRoot))
    {
        spdlog::warn("[logger] Directory or file {} exists already. Contents will be cleared.",
                     logRoot.string());
        fs::remove_all(logRoot);
    }

    if(not fs::create_directories(logRoot))
    {
        throw std::runtime_error("[Logger::Logger] Unable to create root directory for log at " +
                                 logRoot.string());
    }

    return logRoot;
}


} // End of anonymous namespace.


Logger::Logger(std::filesystem::path logRoot, const size_t maxFileSizeInBytes):
    mLogDirectory(checkAndSetupLogDirectory(std::move(logRoot))),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mLockedChannelPtrMap()
{
    spdlog::info("[Logger] Logging to {} with unit file size {}.",
                 mLogDirectory.string(),
                 mMaxFileSizeInBytes);
}


void Logger::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
    auto& lockedChannel = acquireChannel(channelId);
    lockedChannel([&](Channel& channel) { channel.writeFrame(data); });
}


void Logger::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
    auto& lockedChannel = acquireChannel(channelId);
    lockedChannel([&](Channel& channel) { channel.writeFrame(data); });
}


Logger::LockedChannel& Logger::acquireChannel(const ChannelId channelId)
{
    return mLockedChannelPtrMap(
        [&](ChannelPtrMap& channelPtrMap) -> LockedChannel&
        {
            if(not channelPtrMap.contains(channelId))
            {
                channelPtrMap.try_emplace(
                    channelId,
                    std::make_unique<LockedChannel>(mLogDirectory / toHexString(channelId),
                                                    mMaxFileSizeInBytes));
            }

            return *channelPtrMap.at(channelId);
        });
}


const std::filesystem::path& Logger::path() const noexcept
{
    return mLogDirectory;
}

} // End of namespace naksh::logger
