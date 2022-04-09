////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "memoryCrate.hpp"

#include <filesystem>

namespace naksh::logger
{


class ChannelReader
{
public:
    explicit ChannelReader(std::filesystem::path logRoot);

    ChannelReader(const ChannelReader&) = delete;

    ChannelReader(ChannelReader&& channelReader) noexcept;

    ~ChannelReader();

    ChannelReader& operator=(const ChannelReader&) = delete;

    ChannelReader& operator=(ChannelReader&& channelReader) noexcept;

    [[nodiscard]] MemoryCrate read();

private:
    class ChannelReaderImpl;
    std::unique_ptr<ChannelReaderImpl> mChannelReaderImpl;
};

} // namespace naksh::logger
