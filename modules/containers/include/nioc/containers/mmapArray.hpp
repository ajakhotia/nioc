////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "file.hpp"
#include "mapping.hpp"
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <span>
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
/// Non-copyable; move-constructible. The mapped bytes never change address, so a move transfers
/// the mapping and file descriptor while every element reference stays valid; the moved-from
/// array is empty. Destruction unmaps the region and closes the file. It is not thread-safe;
/// synchronize concurrent access externally. Other processes mapping the same file share the same
/// bytes.
///
/// @tparam ValueType Element type. Must be trivially copyable and have no top-level cv-qualifiers.
///
/// @see MmapConstArray for read-only mapping of an existing file, File, Mapping
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

  /// Contiguous, random-access iterators over the elements.
  using iterator = std::span<ValueType>::iterator;
  using const_iterator = std::span<const ValueType>::iterator;

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
    mFile{File::create(std::move(path), count * sizeof(ValueType))},
    mMapping{Mapping::readWrite(mFile, count * sizeof(ValueType))}
  {
  }

  MmapArray(const MmapArray&) = delete;

  /// @brief Take over @p other's mapping and file descriptor, leaving @p other empty and fit only
  /// for destruction.
  MmapArray(MmapArray&&) noexcept = default;

  ~MmapArray() = default;

  MmapArray& operator=(const MmapArray&) = delete;

  MmapArray& operator=(MmapArray&&) noexcept = delete;

  /// @brief Pointer to the first element; `const`-qualified to match @p self. Equals end() when
  /// empty. Valid for the array's lifetime.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return self.elements().data();
  }

  /// @brief Reference to the element at @p index; `const`-qualified to match @p self.
  ///
  /// @param index Element position. Not bounds-checked; must be less than size().
  [[nodiscard]] decltype(auto) operator[](this auto&& self, const size_type index) noexcept
  {
    return *std::next(self.begin(), static_cast<difference_type>(index));
  }

  /// @brief Reference to the element at @p index, bounds-checked; `const`-qualified to match
  /// @p self.
  ///
  /// @param index Element position.
  ///
  /// @throws std::out_of_range if @p index is not less than size().
  [[nodiscard]] decltype(auto) at(this auto&& self, const size_type index)
  {
    if(index >= self.size())
    {
      common::throwException<std::out_of_range>(
          "Index {} is out of range for an array of size {}.",
          index,
          self.size());
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return std::forward<decltype(self)>(self)[index];
  }

  /// @brief Iterator to the first element; `const`-qualified to match @p self.
  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.elements().begin();
  }

  /// @brief Iterator one past the last element; `const`-qualified to match @p self.
  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.elements().end();
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
    return mMapping.empty();
  }

  /// @brief The backing file's descriptor, open for the array's lifetime, for kernel calls that
  /// address the file rather than the mapping.
  ///
  /// @see File::nativeHandle
  [[nodiscard]] int nativeHandle() const noexcept
  {
    return mFile.nativeHandle();
  }

  /// @brief Number of elements currently mapped.
  [[nodiscard]] size_type size() const noexcept
  {
    return mMapping.size() / sizeof(ValueType);
  }

  /// @brief Evict from memory the pages lying entirely within the elements [@p first, @p last).
  ///
  /// Elements remain readable: an evicted page transparently re-reads from the file on its next
  /// access. Pages only partly covered stay resident, so a short element range may evict nothing.
  ///
  /// @param first The first element of the range; an iterator of this array.
  ///
  /// @param last One past the last element of the range; an iterator of this array.
  ///
  /// @see Mapping::evict
  template<std::contiguous_iterator Iterator>
    requires std::is_same_v<std::iter_value_t<Iterator>, ValueType>
  void evict(const Iterator first, const Iterator last) const noexcept
  {
    mMapping.evict(std::as_bytes(std::span{first, last}));
  }

  /// @brief Truncate or extend the on-disk backing file to @p count elements; does not remap.
  ///
  /// Only the file's length changes. The mapping is untouched, so size(), data(), and the
  /// iterator range keep their original element count and stay valid. Typically used to trim
  /// trailing slack before destruction. On failure, logs an error and leaves the file unchanged.
  ///
  /// @param count New element count on disk. May be larger or smaller than the mapped count.
  void resize(const size_type count) noexcept
  {
    mFile.resize(count * sizeof(ValueType));
  }

private:
  /// The elements as one contiguous span over the mapping; `const`-qualified to match @p self.
  [[nodiscard]] auto elements(this auto&& self) noexcept
  {
    return common::startLifetimeAsArray<ValueType>(self.mMapping.bytes());
  }

  /// The backing file, created read-write and sized to the element count.
  File mFile;

  /// The read-write mapping of the whole file; every element access reads or writes through it.
  Mapping mMapping;
};

} // namespace nioc::containers
