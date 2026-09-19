////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "file.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace nioc::containers
{

/// @brief Owned mapped bytes: a shared (`MAP_SHARED`) memory mapping of a byte range of a File,
/// unmapped on destruction. The File need only outlive construction; the kernel keeps the
/// mapping alive on its own.
///
/// A contiguous range of bytes: iterate it or index it through bytes(). Writes through a
/// read-write mapping reach the file and every other mapping of it; a read-only mapping faults
/// on write. A zero-length range yields an empty mapping.
///
/// Non-copyable; move-constructible. The mapped bytes never change address, so a move transfers
/// the mapping while every previously obtained view stays valid; the moved-from mapping is
/// empty. Not thread-safe.
///
/// @see File, MmapArray, MmapConstArray
class Mapping
{
public:
  using value_type = std::byte;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = std::span<std::byte>::iterator;
  using const_iterator = std::span<const std::byte>::iterator;

  /// @brief Map @p byteCount bytes of @p file starting at @p offset, read-only.
  ///
  /// @param offset Byte offset into the file; a multiple of the page size.
  ///
  /// @throws std::runtime_error if the range cannot be mapped.
  [[nodiscard]] static Mapping readOnly(
      const File& file,
      std::size_t byteCount,
      std::int64_t offset = 0);

  /// @brief Map @p byteCount bytes of @p file starting at @p offset, read-write.
  ///
  /// @param offset Byte offset into the file; a multiple of the page size.
  ///
  /// @throws std::runtime_error if the range cannot be mapped.
  [[nodiscard]] static Mapping readWrite(
      const File& file,
      std::size_t byteCount,
      std::int64_t offset = 0);

  Mapping(const Mapping&) = delete;

  Mapping(Mapping&& other) noexcept;

  ~Mapping();

  Mapping& operator=(const Mapping&) = delete;

  Mapping& operator=(Mapping&&) noexcept = delete;

  /// @brief The mapped bytes, mutable; valid until the mapping is destroyed.
  [[nodiscard]] std::span<std::byte> bytes() noexcept;

  /// @brief The mapped bytes, const; valid until the mapping is destroyed.
  [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

  /// @brief Pointer to the first mapped byte; `const`-qualified to match @p self.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return self.bytes().data();
  }

  /// @brief Iterator to the first mapped byte; `const`-qualified to match @p self.
  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.bytes().begin();
  }

  /// @brief Iterator one past the last mapped byte; `const`-qualified to match @p self.
  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.bytes().end();
  }

  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return bytes().begin();
  }

  [[nodiscard]] const_iterator cend() const noexcept
  {
    return bytes().end();
  }

  /// @brief Whether the mapping holds zero bytes.
  [[nodiscard]] bool empty() const noexcept;

  /// @brief Number of mapped bytes.
  [[nodiscard]] std::size_t size() const noexcept;

  /// @brief Evict from memory the pages that lie entirely within @p range.
  ///
  /// The kernel works in whole pages, so a page only partly covered by the range stays resident:
  /// eviction never touches a byte outside the range. A range narrower than a page evicts nothing.
  /// On a file-backed mapping an evicted page transparently re-reads from the file on its next
  /// access, so no content is ever lost.
  ///
  /// A hint that never fails the caller: on error the mapping logs at debug level and continues.
  /// A range that does not lie within bytes() is ignored, as is an empty range or an empty
  /// mapping.
  ///
  /// @param range A subspan of bytes() whose interior pages to evict.
  void evict(std::span<const std::byte> range) const noexcept;

private:
  explicit Mapping(std::span<std::byte> bytes) noexcept;

  /// The mapped bytes; empty in a moved-from mapping. The destructor unmaps this range.
  std::span<std::byte> mBytes;
};

} // namespace nioc::containers
