////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "memoryCrateImpl.hpp"


namespace nioc::logger
{

MemoryCrate::MemoryCrate(std::shared_ptr<MemoryCrateImpl> memoryCrateImplPtr):
    mMemoryCrateImplPtr(std::move(memoryCrateImplPtr))
{
}

MemoryCrate::MemoryCrate(const MemoryCrate& memoryCrate) = default;

MemoryCrate::MemoryCrate(MemoryCrate&& memoryCrate) noexcept = default;

MemoryCrate::~MemoryCrate() = default;

MemoryCrate& MemoryCrate::operator=(MemoryCrate&& memoryCrate) noexcept = default;

MemoryCrate& MemoryCrate::operator=(const MemoryCrate& memoryCrate) = default;


std::span<const std::byte> MemoryCrate::span() const
{
    return mMemoryCrateImplPtr->span();
}


} // namespace nioc::logger
