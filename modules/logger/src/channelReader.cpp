////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "channelReaderImpl.hpp"

namespace naksh::logger
{

ChannelReader::ChannelReader(std::filesystem::path logRoot):
    mChannelReaderImpl(std::make_unique<ChannelReaderImpl>(std::move(logRoot)))
{
}

ChannelReader::ChannelReader(ChannelReader&& channelReader) noexcept = default;

ChannelReader::~ChannelReader() = default;

ChannelReader& ChannelReader::operator=(ChannelReader&& channelReader) noexcept = default;


MemoryCrate ChannelReader::read()
{
    return mChannelReaderImpl->read();
}


} // namespace naksh::logger
