////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <type_traits>

namespace nioc::containers
{

/// @brief Owns a file-backed, shared (`MAP_SHARED`) memory mapping of a contiguous range of bytes.
///
/// Writes through the mapping reach the backing file and any other mapping of it. Choose a mode at
/// construction: the two-argument constructor creates a writable region of a fixed size; the
/// one-argument constructor maps an existing file read-only. Access the bytes through bytes() or
/// data(); they stay valid until the region is destroyed, which unmaps the memory and closes the
/// file.
///
/// Example:
///
///     nioc::containers::MmapRegion region{"/tmp/buffer.bin", 4096};
///     std::span<std::byte> bytes = region.bytes();
///     // ... fill bytes ...
///     region.resize(usedBytes);  // trim the file before it is unmapped
///
/// Non-copyable and non-movable: a region is pinned to its mapped address.
///
/// Not thread-safe. Synchronize concurrent access to the mapped bytes externally.
class MmapRegion
{
public:
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
  /// write protection.
  ///
  /// @param path Existing file to map. Must already exist.
  ///
  /// @throws std::runtime_error if the file cannot be opened, stat'd, or mapped.
  explicit MmapRegion(std::filesystem::path path);

  MmapRegion(const MmapRegion&) = delete;

  MmapRegion(MmapRegion&&) noexcept = delete;

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

  /// @brief Pointer to the first mapped byte; `const`-qualified to match @p self.
  ///
  /// Returns `std::byte*` on a non-const region, `const std::byte*` on a const one.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return self.bytes().data();
  }

  /// @brief Path of the backing file.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

  /// @brief Whether the region maps zero bytes.
  [[nodiscard]] bool empty() const noexcept;

  /// @brief Number of mapped bytes.
  [[nodiscard]] std::size_t size() const noexcept;

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
  const std::filesystem::path mPath;

  /// Descriptor of the open backing file. Held open for the region's lifetime and closed by the
  /// destructor; resize() truncates the file through it.
  const int mFileDescriptor;

  /// The mapped bytes. Its length is the mapping size and never changes after construction; the
  /// destructor unmaps this range.
  const std::span<std::byte> mBytes;
};

/// @brief Reinterpret a span of bytes as a pointer to @p ValueType, preserving const-ness.
///
/// Returns the span's data address typed as `ValueType*`, or `const ValueType*` when the bytes are
/// const. Does no alignment, size, or lifetime checking; the caller guarantees the bytes hold a
/// valid @p ValueType.
///
/// Example:
///
///     auto* header = asElementPointer<Header>(region.bytes());
///
/// @tparam ValueType Element type to view the bytes as. Name it explicitly.
///
/// @tparam Byte Deduced from @p bytes; `std::byte` or `const std::byte`. Its const-ness selects
/// the result's const-ness.
///
/// @param bytes Bytes to reinterpret. Must hold a valid @p ValueType; not checked.
template<typename ValueType, typename Byte>
  requires std::is_same_v<std::remove_const_t<Byte>, std::byte>
[[nodiscard]] auto* asElementPointer(std::span<Byte> bytes) noexcept
{
  using Element = std::conditional_t<std::is_const_v<Byte>, const ValueType, ValueType>;
  using VoidPointer = std::conditional_t<std::is_const_v<Byte>, const void*, void*>;
  // Cast through void* (not reinterpret_cast) to view the mapped bytes as ValueType without a
  // cast-align warning. TODO(ajakhotia): switch to std::start_lifetime_as_array<ValueType> once
  // libstdc++ defines __cpp_lib_start_lifetime_as.
  // NOLINTNEXTLINE(bugprone-casting-through-void)
  return static_cast<Element*>(static_cast<VoidPointer>(bytes.data()));
}

} // namespace nioc::containers
