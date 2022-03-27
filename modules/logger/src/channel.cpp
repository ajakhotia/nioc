////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

#include <naksh/logger/channel.hpp>
#include <numeric>
#include <spdlog/spdlog.h>

namespace naksh::logger
{
namespace
{

std::filesystem::path setupLogRoot(std::filesystem::path logRoot)
{
    namespace fs = std::filesystem;

    if(fs::exists(logRoot))
    {
        throw std::logic_error("[Channel::Channel] Directory or file" + logRoot.string() +
                               " exists already.");
    }

    if(not fs::create_directories(logRoot))
    {
        throw std::runtime_error("[Channel::Channel] Unable to create directory at " +
                                 logRoot.string() + ".");
    }

    return logRoot;
}

} // namespace


Channel::Channel(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
    mLogRoot(setupLogRoot(std::move(logRoot))),
    mIndexFile(mLogRoot / kIndexFileName),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mRollCounter(-1),
    mActiveLogRoll(nextRollFilePath())
{
}


void Channel::writeFrame(const ConstByteSpan& data)
{
    rollAndIndex(data.size_bytes());
    writeToFile(mActiveLogRoll, data);
}


void Channel::writeFrame(const std::vector<ConstByteSpan>& dataCollection)
{
    rollAndIndex(computeTotalSizeInBytes(dataCollection));
    for(const auto& data: dataCollection)
    {
        writeToFile(mActiveLogRoll, data);
    }
}


void Channel::rollAndIndex(std::size_t requiredSizeInBytes)
{
    if(requiredSizeInBytes == 0U)
    {
        return;
    }

    // Advance to next roll if there isn't enough space in the current roll.
    if(not fileHasSpace(mActiveLogRoll, requiredSizeInBytes, mMaxFileSizeInBytes))
    {
        mActiveLogRoll = std::ofstream(nextRollFilePath());
    }

    // Write the index of the roll and the position of the upcoming data blob w.r.t to the start
    // of the roll to the index file.
    const auto position = mActiveLogRoll.tellp();

    if(position >= 0)
    {
        // Write the roll id and position to the index file
        writeToFile(mIndexFile, static_cast<size_t>(mRollCounter));
        writeToFile(mIndexFile, position);

        // Write the size of the upcoming data blob to the current roll.
        writeToFile(mActiveLogRoll, requiredSizeInBytes);
    }
    else
    {
        throw std::runtime_error("[Channel::index] Unable to retrieve the write position");
    }
}


std::filesystem::path Channel::nextRollFilePath()
{
    static constexpr auto kPadding = 20UL;
    return mLogRoot / (kRollFileNamePrefix + padString(std::to_string(++mRollCounter), kPadding) +
                       kRollFileNameExtension);
}


} // namespace naksh::logger
