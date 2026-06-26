////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "mmapRegion.hpp"
#include <cstddef>
#include <filesystem>
#include <nioc/common/exception.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nioc::containers
{

/// @brief A writable, file-backed array of `ValueType` whose storage is a memory-mapped file.
///
/// Use this as a fixed-size, std-like contiguous container that persists to disk. Reads and writes
/// go straight to the mapping, so every element write reaches the backing file. The constructor
/// creates or truncates the file and maps it read-write, so any prior file contents are discarded
/// and every element starts zero-filled. Element access is unchecked; an out-of-range index is
/// undefined behavior.
///
/// Example:
///
///     // 1024 doubles backed by /data/scratch.bin, all initially 0.0.
///     MmapArray<double> a{"/data/scratch.bin", 1024};
///     a[0] = 3.14; // persisted to the file
///     for (auto x : a) { ... }
///
/// The array is pinned to its mapped address: it is neither copyable nor movable, since it owns the
/// mapping and the file descriptor. Destruction unmaps the region and closes the file. It is not
/// thread-safe; synchronize concurrent access externally. Other processes mapping the same file
/// share the same bytes.
///
/// @tparam ValueType Element type. Must be trivially copyable and have no top-level cv-qualifiers.
///
/// @see MmapConstArray for read-only mapping of an existing file, MmapRegion
template<typename ValueType>
  requires std::is_trivially_copyable_v<ValueType> and
           std::is_same_v<ValueType, std::remove_cv_t<ValueType>>
class MmapArray
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = ValueType&;
  using const_reference = const ValueType&;
  using pointer = ValueType*;
  using const_pointer = const ValueType*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  /// @brief Create or truncate the file at @p path to hold @p count elements and map it read-write.
  ///
  /// Creates any missing parent directories. Discards existing file contents and zero-fills the new
  /// storage.
  ///
  /// @param path Backing file. Created if absent, truncated if present.
  ///
  /// @param count Number of elements. Must be non-zero; a zero-length mapping is rejected.
  ///
  /// @throws std::runtime_error If the file cannot be created, sized, or mapped.
  MmapArray(std::filesystem::path path, const size_type count):
    mRegion{std::move(path), count * sizeof(ValueType)}
  {
  }

  MmapArray(const MmapArray&) = delete;

  MmapArray(MmapArray&&) noexcept = delete;

  ~MmapArray() = default;

  MmapArray& operator=(const MmapArray&) = delete;

  MmapArray& operator=(MmapArray&&) noexcept = delete;

  /// @brief Pointer to the first element. Const-qualified when called on a `const` array.
  ///
  /// Valid for the array's lifetime.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return asElementPointer<ValueType>(self.mRegion.bytes());
  }

  /// @brief Reference to the element at @p index. Const-qualified when called on a `const` array.
  ///
  /// @param index Element position. Not bounds-checked; must be less than `size()`.
  [[nodiscard]] decltype(auto) operator[](this auto&& self, const size_type index) noexcept
  {
    return self.data()[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Reference to the element at @p index, bounds-checked. Const-qualified when called on a
  /// `const` array.
  ///
  /// @param index Element position.
  ///
  /// @throws std::out_of_range if @p index is not less than `size()`.
  [[nodiscard]] decltype(auto) at(this auto&& self, const size_type index)
  {
    if(index >= self.size())
    {
      common::throwException<std::out_of_range>(
          "Index {} is out of range for an array of size {}.",
          index,
          self.size());
    }
    return std::forward<decltype(self)>(self)[index];
  }

  /// @brief Iterator to the first element. Const-qualified when called on a `const` array.
  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.data();
  }

  /// @brief Iterator one past the last element. Const-qualified when called on a `const` array.
  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.data() + self.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Const iterator to the first element.
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// @brief Const iterator one past the last element.
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// @brief True if the array holds no elements.
  [[nodiscard]] bool empty() const noexcept
  {
    return mRegion.empty();
  }

  /// @brief Number of elements currently mapped.
  [[nodiscard]] size_type size() const noexcept
  {
    return mRegion.size() / sizeof(ValueType);
  }

  /// @brief Truncate or extend the on-disk backing file to @p count elements; does not remap.
  ///
  /// Only the file's length changes. The mapping is untouched, so `size()`, `data()`, and the
  /// iterator range keep their original element count and stay valid. Typically used to trim
  /// trailing slack before destruction. On failure, logs an error and leaves the file unchanged.
  ///
  /// @param count New element count on disk. May be larger or smaller than the mapped count.
  void resize(const size_type count) noexcept
  {
    mRegion.resize(count * sizeof(ValueType));
  }

private:
  /// The read-write memory mapping and its backing file. Owns both; sizing this region in bytes
  /// defines the element count, and every element access reads or writes through it.
  MmapRegion mRegion;
};

} // namespace nioc::containers
