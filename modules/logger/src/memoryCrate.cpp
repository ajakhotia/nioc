////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "memoryCrateImpl.hpp"


namespace naksh::logger
{

MemoryCrate::MemoryCrate(): mMemoryCrateImpl(std::make_unique<MemoryCrate::MemoryCrateImpl>()) {}

MemoryCrate::MemoryCrate(const MemoryCrate&) = default;

MemoryCrate::MemoryCrate(MemoryCrate&&) noexcept = default;

MemoryCrate::~MemoryCrate() = default;

MemoryCrate& MemoryCrate::operator=(MemoryCrate&&) noexcept = default;

MemoryCrate& MemoryCrate::operator=(const MemoryCrate&) = default;


std::span<const std::byte> MemoryCrate::span() const
{
    return mMemoryCrateImpl->span();
}


MemoryCrate::ChannelId MemoryCrate::channelId() const
{
    return mMemoryCrateImpl->channelId();
}


} // namespace naksh::logger
