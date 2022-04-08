////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <naksh/logger/memoryCrate.hpp>

namespace naksh::logger
{


class MemoryCrate::MemoryCrateImpl
{
public:
    MemoryCrateImpl();

    MemoryCrateImpl(const MemoryCrateImpl&) = default;

    MemoryCrateImpl(MemoryCrateImpl&&) = default;

    ~MemoryCrateImpl() = default;

    MemoryCrateImpl& operator=(const MemoryCrateImpl&) = default;

    MemoryCrateImpl& operator=(MemoryCrateImpl&&) = default;

    [[nodiscard]] std::span<const std::byte> span() const;

    [[nodiscard]] ChannelId channelId() const;
};


} // namespace naksh::logger
