////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "loggerImpl.hpp"
#include <nioc/logger/logger.hpp>

namespace nioc::logger
{

Logger::Logger(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
    mLoggerImpl(std::make_unique<LoggerImpl>(std::move(logRoot), maxFileSizeInBytes))
{
}


Logger::Logger(Logger&& rhs) noexcept = default;

Logger::~Logger() = default;


void Logger::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
    mLoggerImpl->write(channelId, data);
}

void Logger::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
    mLoggerImpl->write(channelId, data);
}

const std::filesystem::path& Logger::path() const noexcept
{
    return mLoggerImpl->path();
}

} // End of namespace nioc::logger
