////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <span>

namespace nioc::chronicle
{

/// @brief A read-only view over a block of bytes.
///
/// Copies and moves are cheap. All copies share the same bytes.
class MemoryCrate
{
public:
  class MmapMemoryCrate;

  /// @brief Constructs a MemoryCrate backed by @p mmapMemoryCratePtr.
  /// @param mmapMemoryCratePtr Backing storage for the bytes.
  explicit MemoryCrate(std::shared_ptr<MmapMemoryCrate> mmapMemoryCratePtr);

  MemoryCrate(const MemoryCrate& memoryCrate);

  MemoryCrate(MemoryCrate&& memoryCrate) noexcept;

  ~MemoryCrate();

  MemoryCrate& operator=(const MemoryCrate& memoryCrate);

  MemoryCrate& operator=(MemoryCrate&& memoryCrate) noexcept;

  /// @brief Returns a read-only view of the bytes.
  [[nodiscard]] std::span<const std::byte> span() const;

private:
  std::shared_ptr<MmapMemoryCrate> mMmapMemoryCratePtr;
};


} // namespace nioc::chronicle
