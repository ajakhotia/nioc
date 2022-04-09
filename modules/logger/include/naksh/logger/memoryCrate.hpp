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

    class MemoryCrateImpl;

    explicit MemoryCrate(std::shared_ptr<MemoryCrateImpl> memoryCrateImplPtr);

    MemoryCrate(const MemoryCrate& memoryCrate);

    MemoryCrate(MemoryCrate&& memoryCrate) noexcept;

    ~MemoryCrate();

    MemoryCrate& operator=(const MemoryCrate& memoryCrate);

    MemoryCrate& operator=(MemoryCrate&& memoryCrate) noexcept;

    [[nodiscard]] std::span<const std::byte> span() const;

private:
    std::shared_ptr<MemoryCrateImpl> mMemoryCrateImplPtr;
};


} // namespace naksh::logger
