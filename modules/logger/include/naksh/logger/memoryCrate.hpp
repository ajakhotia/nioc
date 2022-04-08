////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <span>

namespace naksh::logger
{

class MemoryCrate
{
public:
    using ChannelId = std::uint64_t;

    MemoryCrate();

    MemoryCrate(const MemoryCrate&);

    MemoryCrate(MemoryCrate&&) noexcept;

    ~MemoryCrate();

    MemoryCrate& operator=(const MemoryCrate&);

    MemoryCrate& operator=(MemoryCrate&&) noexcept;

    [[nodiscard]] std::span<const std::byte> span() const;

    [[nodiscard]] ChannelId channelId() const;

private:
    class MemoryCrateImpl;
    std::shared_ptr<MemoryCrateImpl> mMemoryCrateImpl;
};


} // namespace naksh::logger
