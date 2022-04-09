////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <naksh/logger/logger.hpp>
#include <spdlog/spdlog.h>

namespace naksh::logger
{
namespace
{
namespace fs = std::filesystem;

fs::path checkAndSetupLogDirectory(fs::path logRoot)
{
    logRoot /= (timeAsFormattedString(std::chrono::system_clock::now())) + "_" +
               boost::uuids::to_string(boost::uuids::random_generator_pure()());

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
    mLockedSequenceFile(mLogDirectory / kSequenceFileName),
    mLockedChannelPtrMap()
{
    spdlog::info("[Logger] Logging to {} with unit file size {}.",
                 mLogDirectory.string(),
                 mMaxFileSizeInBytes);
}


void Logger::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
    // TODO: This can be improved to use fewer locks and avoid race conditions.
    mLockedSequenceFile(
        [&](std::ofstream& sequenceFile) {
            ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{channelId}); });

    auto& lockedChannel = acquireChannel(channelId);
    lockedChannel([&](Channel& channel) { channel.writeFrame(data); });
}


void Logger::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
    // TODO: This can be improved to use fewer locks and avoid race conditions.
    mLockedSequenceFile(
        [&](std::ofstream& sequenceFile) {
            ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{channelId}); });

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
