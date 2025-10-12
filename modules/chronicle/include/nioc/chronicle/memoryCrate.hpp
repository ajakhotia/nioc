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

/// @brief A container for chronicle data.
///
/// Provides read-only access to data from a chronicle entry.
class MemoryCrate
{
public:
  class MmapMemoryCrate;

  /// @brief Constructs a MemoryCrate.
  /// @param mmapMemoryCratePtr Implementation pointer.
  explicit MemoryCrate(std::shared_ptr<MmapMemoryCrate> mmapMemoryCratePtr);

  MemoryCrate(const MemoryCrate& memoryCrate);

  MemoryCrate(MemoryCrate&& memoryCrate) noexcept;

  ~MemoryCrate();

  MemoryCrate& operator=(const MemoryCrate& memoryCrate);

  MemoryCrate& operator=(MemoryCrate&& memoryCrate) noexcept;

  /// @brief Gets a read-only view of the data.
  /// @return Span of bytes containing the data.
  [[nodiscard]] std::span<const std::byte> span() const;

private:
  std::shared_ptr<MmapMemoryCrate> mMmapMemoryCratePtr;
};


} // namespace nioc::chronicle
