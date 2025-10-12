////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapReader.hpp"
#include "streamWriter.hpp"
#include <nioc/chronicle/chronicle.hpp>

namespace nioc::chronicle
{

Writer::Writer(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
    mStreamWriter(std::make_unique<StreamWriter>(std::move(logRoot), maxFileSizeInBytes))
{
}

Writer::Writer(Writer&& rhs) noexcept = default;

Writer::~Writer() = default;

void Writer::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
  mStreamWriter->write(channelId, data);
}

void Writer::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
  mStreamWriter->write(channelId, data);
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mStreamWriter->path();
}

Reader::Reader(std::filesystem::path logRoot):
    mMmapReader(std::make_unique<MmapReader>(std::move(logRoot)))
{
}

Reader::Reader(Reader&&) noexcept = default;

Reader::~Reader() = default;

Reader& Reader::operator=(Reader&&) noexcept = default;

Entry Reader::read()
{
  return mMmapReader->read();
}

} // End of namespace nioc::chronicle
