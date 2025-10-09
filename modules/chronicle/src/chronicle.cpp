////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "logReaderImpl.hpp"
#include "loggerImpl.hpp"
#include <nioc/chronicle/chronicle.hpp>

namespace nioc::chronicle
{

Writer::Writer(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
    mLoggerImpl(std::make_unique<LoggerImpl>(std::move(logRoot), maxFileSizeInBytes))
{
}

Writer::Writer(Writer&& rhs) noexcept = default;

Writer::~Writer() = default;

void Writer::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
  mLoggerImpl->write(channelId, data);
}

void Writer::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
  mLoggerImpl->write(channelId, data);
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mLoggerImpl->path();
}

Reader::Reader(std::filesystem::path logRoot):
    mLogReaderImpl(std::make_unique<LogReaderImpl>(std::move(logRoot)))
{
}

Reader::Reader(Reader&&) noexcept = default;

Reader::~Reader() = default;

Reader& Reader::operator=(Reader&&) noexcept = default;

Entry Reader::read()
{
  return mLogReaderImpl->read();
}

} // End of namespace nioc::chronicle
