////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <memory>
#include <span>

namespace nioc::chronicle
{

/// @brief An immutable, contiguous view of bytes that keeps its backing alive.
///
/// Cheap to copy; all copies share the same bytes. The backing storage is opaque to the holder. The
/// bytes stay valid for as long as any copy of the crate lives.
class Crate
{
public:
  /// @brief Constructs an empty crate.
  Crate() noexcept = default;

  /// @brief Constructs a crate over @p span, keeping @p backing alive for as long as it lives.
  ///
  /// @param backing Owner of the storage the @p span points into.
  ///
  /// @param span The bytes this crate exposes.
  Crate(std::shared_ptr<const void> backing, std::span<const std::byte> span) noexcept;

  /// @brief Returns a read-only view of the bytes.
  [[nodiscard]] std::span<const std::byte> span() const noexcept;

private:
  std::shared_ptr<const void> mBacking;
  std::span<const std::byte> mSpan;
};

} // namespace nioc::chronicle
