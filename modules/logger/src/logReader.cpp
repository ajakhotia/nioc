////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <naksh/logger/logReader.hpp>

namespace naksh::logger
{
namespace fs = std::filesystem;


namespace
{

fs::path validatePath(fs::path logRoot)
{
    if(not fs::exists(logRoot))
    {
        throw std::invalid_argument("[LogReader::LogReader] Directory " + logRoot.string() +
                                    " does not exist.");
    }

    return logRoot;
}

} // namespace


LogEntry::LogEntry(const ChannelId channelId, MemoryCrate memoryCrate):
    mChannelId(channelId),
    mMemoryCrate(std::move(memoryCrate))
{
}


LogReader::LogReader(std::filesystem::path logRoot): mLogRoot(validatePath(std::move(logRoot))) {}


LogEntry LogReader::read() {}


} // namespace naksh::logger
