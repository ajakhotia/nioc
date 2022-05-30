////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "loggerImpl.hpp"
#include <naksh/logger/logger.hpp>

namespace naksh::logger
{

Logger::Logger(std::filesystem::path logRoot, const size_t maxFileSizeInBytes):
    mLoggerImpl(std::make_unique<LoggerImpl>(std::move(logRoot), maxFileSizeInBytes))
{
}


Logger::Logger(Logger&& rhs) noexcept = default;

Logger::~Logger() = default;


void Logger::write(size_t channelId, const std::span<const std::byte>& data)
{
    mLoggerImpl->write(channelId, data);
}

void Logger::write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
    mLoggerImpl->write(channelId, data);
}

const std::filesystem::path& Logger::path() const noexcept
{
    return mLoggerImpl->path();
}

} // End of namespace naksh::logger
