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

/// @brief Owns one memory-mapped region of a file, with a fixed address for its lifetime.
///
/// Opens, maps, and (on destruction) unmaps a file, exposing its bytes. Created writable at a fixed
/// size, or opened over an existing file read-only. The untyped counterpart to @ref MmapArray and
/// @ref MmapConstArray; use it directly when raw bytes are what you want.
class MmapRegion
{
public:
  /// @brief Creates @p path at @p size bytes and maps it read-write.
  ///
  /// Parent directories are created as needed; an existing file is truncated.
  ///
  /// @throws std::runtime_error If the file cannot be created, sized, or mapped.
  MmapRegion(std::filesystem::path path, std::size_t size);

  /// @brief Maps the existing file at @p path read-only; its size becomes the region length.
  ///
  /// @throws std::runtime_error If the file cannot be opened or mapped.
  explicit MmapRegion(std::filesystem::path path);

  MmapRegion(const MmapRegion&) = delete;

  MmapRegion(MmapRegion&&) noexcept = delete;

  ~MmapRegion();

  MmapRegion& operator=(const MmapRegion&) = delete;

  MmapRegion& operator=(MmapRegion&&) noexcept = delete;

  /// @brief Returns the mapped bytes; a const region yields a read-only view.
  [[nodiscard]] std::span<std::byte> bytes() noexcept;

  [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

  /// @brief Returns the start of the mapped bytes; a const region yields a read-only pointer.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return self.bytes().data();
  }

  /// @brief Returns the path of the backing file.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

  /// @brief Returns whether the region maps no bytes.
  [[nodiscard]] bool empty() const noexcept;

  /// @brief Returns the mapped length, in bytes.
  [[nodiscard]] std::size_t size() const noexcept;

  /// @brief Resizes the backing file to @p size bytes. Best-effort; never throws.
  ///
  /// The mapping is unchanged, so accessing bytes at or beyond @p size afterward is undefined; call
  /// this only once those bytes are done with.
  void resize(std::size_t size) noexcept;

private:
  const std::filesystem::path mPath;
  const int mFileDescriptor;
  const std::span<std::byte> mBytes;
};

/// @brief Views mapped @p bytes as a pointer to @p ValueType, preserving const-ness.
///
/// The element type's bytes are its whole value (it is trivially copyable), so the mapped storage
/// is already a valid array of @p ValueType. A const byte span yields a `const ValueType*`.
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
