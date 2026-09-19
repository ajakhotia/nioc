////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <filesystem>

namespace nioc::containers
{

/// @brief An owned open file: the descriptor is held for the object's lifetime and closed on
/// destruction. Obtain one by creating a file of a given size, opening an existing file
/// read-only, or adopting a descriptor the kernel handed out for one of its own objects.
///
/// Non-copyable; move-constructible (the moved-from file holds nothing). Not thread-safe.
///
/// @see Mapping, MmapArray, MmapConstArray
class File
{
public:
  /// @brief Create or truncate the file at @p path to @p size zero-filled bytes, open read-write.
  ///
  /// Missing parent directories are created.
  ///
  /// @param path The file; created if absent, truncated to empty first if it exists.
  ///
  /// @param size Byte length to give the file.
  ///
  /// @throws std::runtime_error if the file cannot be created or sized.
  [[nodiscard]] static File create(std::filesystem::path path, std::size_t size);

  /// @brief Open the existing file at @p path read-only.
  ///
  /// @throws std::runtime_error if the file cannot be opened.
  [[nodiscard]] static File open(std::filesystem::path path);

  /// @brief Adopt an already-open @p descriptor, closing it on destruction. For descriptors that
  /// name no path (a kernel object such as an io_uring instance); path() is then empty.
  explicit File(int descriptor) noexcept;

  File(const File&) = delete;

  File(File&& other) noexcept;

  ~File();

  File& operator=(const File&) = delete;

  File& operator=(File&&) noexcept = delete;

  /// @brief The descriptor, for kernel calls that address the file; -1 in a moved-from file.
  [[nodiscard]] int nativeHandle() const noexcept;

  /// @brief The path the file was created or opened at; empty for an adopted descriptor.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

  /// @brief The file's current length in bytes.
  ///
  /// @throws std::runtime_error if the file cannot be inspected.
  [[nodiscard]] std::size_t size() const;

  /// @brief Truncate or extend the file to @p size bytes. Mappings of the file keep their length;
  /// only the on-disk length changes. On failure logs an error and leaves the file unchanged.
  void resize(std::size_t size) const noexcept;

  /// @brief The path if there is one, else the descriptor number: how diagnostics name the file.
  [[nodiscard]] std::string describe() const;

private:
  /// Path the file was created or opened at; empty for an adopted descriptor.
  std::filesystem::path mPath;

  /// The owned descriptor; -1 once moved from.
  int mDescriptor;

  File(std::filesystem::path path, int descriptor) noexcept;
};

} // namespace nioc::containers
