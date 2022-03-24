////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/logger/channel.hpp>
#include <spdlog/spdlog.h>

namespace naksh::logger
{
namespace
{

constexpr auto kRollFileNamePrefix = "roll";
constexpr auto kRollFileNameExtension = ".nio";

} // namespace

namespace fs = std::filesystem;

Channel::Channel(std::filesystem::path logRoot, const std::size_t rollOverLimitBytes):
    mLogRoot(std::move(logRoot)),
    mRollOverLimitBytes(rollOverLimitBytes),
    mRollCounter(0ULL),
    mActiveLogRoll()
{
    if(fs::exists(mLogRoot))
    {
        throw std::logic_error("[Channel::Channel] Directory or file" + mLogRoot.string() +
                               " exists already. This must be a programming bug.");
    }

    if(not fs::create_directories(mLogRoot))
    {
        throw std::runtime_error("[Channel::Channel] Unable to create root directory at " +
                                 mLogRoot.string() + ".");
    }

    advanceLogRoll();
}


void Channel::write(size_t bufferLength, const void* bufferPtr)
{
    if(not rollHasSpace(bufferLength))
    {
        advanceLogRoll();
    }

    // TODO: Add overflow checks.
    mActiveLogRoll.write(static_cast<const char*>(bufferPtr), static_cast<long>(bufferLength));
}


void Channel::advanceLogRoll()
{
    const auto rollFilePath =
        mLogRoot / (kRollFileNamePrefix + std::to_string(mRollCounter++) + kRollFileNameExtension);

    mActiveLogRoll = std::ofstream{rollFilePath};
}


bool Channel::rollHasSpace(size_t spaceRequired)
{
    return (mRollOverLimitBytes - mActiveLogRoll.tellp()) >= spaceRequired;
}


} // namespace naksh::logger
