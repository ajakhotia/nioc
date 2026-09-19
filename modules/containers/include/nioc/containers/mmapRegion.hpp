////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <nioc/common/typeTraits.hpp>
#include <span>
#include <type_traits>

namespace nioc::containers
{

/// @brief Owns a file-backed, shared (`MAP_SHARED`) memory mapping of a contiguous range of bytes.
///
/// Writes through the mapping reach the backing file and any other mapping of it. Choose a mode at
/// construction: the two-argument constructor creates a writable region of a fixed size; the
/// one-argument constructor maps an existing file read-only. The region is a contiguous range of
/// bytes: iterate it, index it through bytes(), or view it as a typed sequence through
/// elements<T>(). Every view stays valid until the region is destroyed, which unmaps the memory
/// and closes the file.
///
/// Example:
///
///     nioc::containers::MmapRegion region{"/tmp/buffer.bin", 4096};
///     std::span<std::byte> bytes = region.bytes();
///     // ... fill bytes ...
///     region.resize(usedBytes);  // trim the file before it is unmapped
///
/// Non-copyable; move-constructible. The mapped bytes never change address, so a move transfers
/// the mapping and file descriptor to the new owner while every previously obtained view stays
/// valid; the moved-from region is empty and fit only for destruction.
///
/// Not thread-safe. Synchronize concurrent access to the mapped bytes externally.
class MmapRegion
{
public:
  using value_type = std::byte;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  /// Contiguous, random-access iterators over the mapped bytes.
  using iterator = std::span<std::byte>::iterator;
  using const_iterator = std::span<const std::byte>::iterator;

  /// @brief Create or truncate the file at @p path to @p size bytes and map it writable.
  ///
  /// Missing parent directories are created. The mapped bytes are zero-filled.
  ///
  /// @param path Backing file. Created if absent; truncated to empty first if it exists.
  ///
  /// @param size Byte length of the file and mapping. Must be non-zero; the kernel rejects a
  /// zero-length mapping.
  ///
  /// @throws std::runtime_error if the file cannot be created, sized, or mapped.
  MmapRegion(std::filesystem::path path, std::size_t size);

  /// @brief Map the existing file at @p path read-only, sized to the file's current length.
  ///
  /// Use only the const accessors. Writing through the bytes faults because the mapping lacks
  /// write protection. A zero-length file yields an empty region with no mapping.
  ///
  /// @param path Existing file to map. Must already exist.
  ///
  /// @throws std::runtime_error if the file cannot be opened, stat'd, or mapped.
  explicit MmapRegion(std::filesystem::path path);

  MmapRegion(const MmapRegion&) = delete;

  /// @brief Take over @p other's mapping and file descriptor, leaving @p other empty and fit only
  /// for destruction.
  MmapRegion(MmapRegion&& other) noexcept;

  /// @brief Unmap the region and close the backing file descriptor.
  ~MmapRegion();

  MmapRegion& operator=(const MmapRegion&) = delete;

  MmapRegion& operator=(MmapRegion&&) noexcept = delete;

  /// @brief View the mapped region as mutable bytes; valid until the region is destroyed.
  ///
  /// Writing has no effect on a read-only region beyond faulting.
  [[nodiscard]] std::span<std::byte> bytes() noexcept;

  /// @brief View the mapped region as const bytes; valid until the region is destroyed.
  [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

  /// @brief View the mapped bytes as a contiguous sequence of @p ValueType; `const`-qualified to
  /// match @p self.
  ///
  /// The sequence holds size() / sizeof(ValueType) elements; trailing bytes that do not fill an
  /// element are excluded. Does no alignment or lifetime checking; the caller guarantees the
  /// bytes hold valid @p ValueType objects.
  ///
  /// @tparam ValueType Implicit-lifetime element type without top-level cv-qualifiers.
  template<typename ValueType>
    requires common::isImplicitLifetime<ValueType> and
             std::is_same_v<ValueType, std::remove_cv_t<ValueType>>
  [[nodiscard]] auto elements(this auto&& self) noexcept
  {
    return common::startLifetimeAsArray<ValueType>(self.bytes());
  }

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

  /// @brief Const iterator to the first mapped byte.
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return bytes().begin();
  }

  /// @brief Const iterator one past the last mapped byte.
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return bytes().end();
  }

  /// @brief Path of the backing file.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

  /// @brief Whether the region maps zero bytes.
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
  /// A hint that never fails the caller: on error the region logs at debug level and continues.
  /// A range that does not lie within bytes() is ignored, as is an empty range or an empty region.
  ///
  /// @param range A subspan of bytes() whose interior pages to evict.
  void evict(std::span<const std::byte> range) const noexcept;

  /// @brief Truncate or extend the backing file on disk to @p size bytes without remapping.
  ///
  /// The mapping and every span/pointer from bytes() and data() keep their original length and
  /// stay valid; only the file's on-disk length changes. Use this to trim trailing slack from a
  /// writable region before it is destroyed. On failure logs an error and leaves the file
  /// unchanged.
  ///
  /// @param size New on-disk length in bytes.
  void resize(std::size_t size) noexcept;

private:
  /// Path of the backing file, retained for path() and for diagnostics.
  std::filesystem::path mPath;

  /// Descriptor of the open backing file, or -1 in a moved-from region. Held open for the region's
  /// lifetime and closed by the destructor; resize() truncates the file through it.
  int mFileDescriptor;

  /// The mapped bytes; empty in a moved-from region. The destructor unmaps this range.
  std::span<std::byte> mBytes;
};

} // namespace nioc::containers
