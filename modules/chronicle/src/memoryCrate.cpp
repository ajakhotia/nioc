////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapMemoryCrate.hpp"

namespace nioc::chronicle
{

MemoryCrate::MemoryCrate(std::shared_ptr<MmapMemoryCrate> mmapMemoryCratePtr):
    mMmapMemoryCratePtr(std::move(mmapMemoryCratePtr))
{
}

MemoryCrate::MemoryCrate(const MemoryCrate& memoryCrate) = default;

MemoryCrate::MemoryCrate(MemoryCrate&& memoryCrate) noexcept = default;

MemoryCrate::~MemoryCrate() = default;

MemoryCrate& MemoryCrate::operator=(MemoryCrate&& memoryCrate) noexcept = default;

MemoryCrate& MemoryCrate::operator=(const MemoryCrate& memoryCrate) = default;

std::span<const std::byte> MemoryCrate::span() const
{
  return mMmapMemoryCratePtr->span();
}


} // namespace nioc::chronicle
