////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <nioc/chronicle/crate.hpp>
#include <span>
#include <utility>

namespace nioc::chronicle
{

Crate::Crate(std::shared_ptr<const void> backing, const std::span<const std::byte> span) noexcept:
  mBacking{std::move(backing)},
  mSpan{span}
{
}

std::span<const std::byte> Crate::span() const noexcept
{
  return mSpan;
}

} // namespace nioc::chronicle
