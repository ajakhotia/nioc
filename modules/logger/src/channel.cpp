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


Channel::Channel(std::filesystem::path logRoot, const std::uint64_t maxFileSizeInBytes):
    mLogRoot(setupLogRoot(std::move(logRoot))),
    mIndexFile(mLogRoot / kIndexFileName),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mRollCounter(std::numeric_limits<std::uint64_t>::max()),
    mActiveLogRoll(nextRollFilePath())
{
}


void Channel::writeFrame(const ConstByteSpan& data)
{
    const auto sizeInBytes = data.size_bytes();
    rollAndIndex(sizeInBytes);

    // Write the size and the blob to the current roll.
    writeToFile(mActiveLogRoll, sizeInBytes);
    writeToFile(mActiveLogRoll, data);

    // Check if the file is still good.
    if(not mActiveLogRoll.good())
    {
        throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
    }
}


void Channel::writeFrame(const std::vector<ConstByteSpan>& dataCollection)
{
    const auto sizeInBytes = computeTotalSizeInBytes(dataCollection);
    rollAndIndex(sizeInBytes);

    // Write the size and the blob to the current roll.
    writeToFile(mActiveLogRoll, sizeInBytes);
    for(const auto& data: dataCollection)
    {
        writeToFile(mActiveLogRoll, data);
    }

    // Check if the file is still good.
    if(not mActiveLogRoll.good())
    {
        throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
    }
}


void Channel::rollAndIndex(const std::uint64_t requiredSizeInBytes)
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
    if(const auto position = mActiveLogRoll.tellp(); position >= 0)
    {
        // Create and write an IndexEntry to the index file.
        writeToFile(mIndexFile, IndexEntry{mRollCounter, static_cast<std::uint64_t>(position)});
    }
    else
    {
        throw std::runtime_error("[Channel::index] Unable to retrieve the write position");
    }
}


std::filesystem::path Channel::nextRollFilePath()
{
    return mLogRoot / (kRollFileNamePrefix +
                       padString(std::to_string(++mRollCounter), kPaddedRollNumberLength) +
                       kRollFileNameExtension);
}


} // namespace naksh::logger
