////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/logger/logger.hpp>
#include <spdlog/fmt/chrono.h>
#include <spdlog/spdlog.h>

namespace naksh::logger
{
namespace
{
namespace fs = std::filesystem;
using std::chrono::system_clock;


std::string timeNowAsFormattedString()
{
    std::string timeString;

    fmt::format_to(std::back_inserter(timeString),
                   "{:%Y-%m-%dT%H:%M:%S%z}",
                   fmt::localtime(system_clock::now()));

    return timeString;
}


fs::path checkAndSetupLogDirectory(fs::path logRoot)
{
    logRoot /= (timeNowAsFormattedString());

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


Logger::Logger(std::filesystem::path logRoot, const size_t fileSize):
    mLogDirectory(checkAndSetupLogDirectory(std::move(logRoot))), mFileSize(fileSize)
{
    spdlog::info(
        "[Logger] Logging to {} with unit file size {}.", mLogDirectory.string(), mFileSize);
}


void Logger::write(const size_t /* channelId */,
                   const size_t /* bufferLength */,
                   const void* /* bufferPtr */)
{
}

} // End of namespace naksh::logger
