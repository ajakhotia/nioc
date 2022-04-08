////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "memoryCrateImpl.hpp"

namespace naksh::logger
{


MemoryCrate::MemoryCrateImpl::MemoryCrateImpl() {}


std::span<const std::byte> MemoryCrate::MemoryCrateImpl::span() const {}

MemoryCrate::ChannelId MemoryCrate::MemoryCrateImpl::channelId() const {}

} // namespace naksh::logger
