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

/// @brief A read-only, random-access container that views an existing file's bytes as a contiguous
/// sequence of @p ValueType elements, without copying the file into the heap.
///
/// The file is memory-mapped, so the OS pages it in on demand and nothing is read until an element
/// is accessed. The view is fixed-size and immutable: there is no resize and no way to mutate
/// elements. It models a contiguous range, so it works with range-based for loops and standard
/// algorithms.
///
/// Example:
///
///     // Map a file of recorded floats and sum them.
///     MmapConstArray<float> samples{"/data/samples.bin"};
///     float total = 0.0F;
///     for(const float value: samples)
///     {
///       total += value;
///     }
///
/// Non-copyable and non-movable. Every pointer, reference, and iterator it returns stays valid
/// until the container is destroyed. Concurrent reads are safe; behaviour is unspecified if the
/// file is changed through another handle while it is mapped.
///
/// @tparam ValueType The element type the file's bytes are reinterpreted as. Must be trivially
/// copyable and have no top-level cv-qualifiers; elements are mapped, never constructed.
///
/// @see MmapRegion
template<typename ValueType>
  requires std::is_trivially_copyable_v<ValueType> and
           std::is_same_v<ValueType, std::remove_cv_t<ValueType>>
class MmapConstArray
{
public:
  using value_type = ValueType;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  /// Reference to an element; always const since the view is read-only.
  using const_reference = const ValueType&;

  /// Pointer to an element; always const since the view is read-only.
  using const_pointer = const ValueType*;

  /// Iterator over elements; a raw const pointer, so the range is contiguous.
  using const_iterator = const_pointer;

  /// @brief Map the existing file at @p path read-only and view its bytes as a sequence of
  /// @p ValueType.
  ///
  /// @param path Path to an existing file. Its byte length must be a whole multiple of
  /// sizeof(ValueType).
  ///
  /// @throws std::runtime_error if the file cannot be opened or mapped, or if its byte length is
  /// not a whole multiple of sizeof(ValueType).
  explicit MmapConstArray(std::filesystem::path path): mRegion{std::move(path)}
  {
    if(mRegion.size() % sizeof(ValueType) != 0)
    {
      common::throwException<std::runtime_error>(
          "{} is {} bytes, not a whole multiple of the {}-byte element size",
          mRegion.path().string(),
          mRegion.size(),
          sizeof(ValueType));
    }
  }

  MmapConstArray(const MmapConstArray&) = delete;

  MmapConstArray(MmapConstArray&&) noexcept = delete;

  ~MmapConstArray() = default;

  MmapConstArray& operator=(const MmapConstArray&) = delete;

  MmapConstArray& operator=(MmapConstArray&&) noexcept = delete;

  /// @brief Pointer to the first element. Equals end() when empty.
  ///
  /// Stays valid for the container's lifetime.
  [[nodiscard]] const_pointer data() const noexcept
  {
    return asElementPointer<ValueType>(mRegion.bytes());
  }

  /// @brief Element at @p index.
  ///
  /// @param index Position to read. Unchecked: must be less than size(), otherwise behaviour is
  /// undefined.
  [[nodiscard]] const_reference operator[](const size_type index) const noexcept
  {
    return data()[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Element at @p index, bounds-checked.
  ///
  /// @param index Position to read.
  ///
  /// @throws std::out_of_range if @p index is not less than `size()`.
  [[nodiscard]] const_reference at(const size_type index) const
  {
    if(index >= size())
    {
      common::throwException<std::out_of_range>(
          "Index {} is out of range for an array of size {}.",
          index,
          size());
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return (*this)[index];
  }

  /// Iterator to the first element.
  [[nodiscard]] const_iterator begin() const noexcept
  {
    return data();
  }

  /// Iterator one past the last element.
  [[nodiscard]] const_iterator end() const noexcept
  {
    return data() + size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// Iterator to the first element; same as begin().
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// Iterator one past the last element; same as end().
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// True if the view holds no elements.
  [[nodiscard]] bool empty() const noexcept
  {
    return mRegion.empty();
  }

  /// @brief Number of elements.
  ///
  /// Equals the file's byte length divided by sizeof(ValueType).
  [[nodiscard]] size_type size() const noexcept
  {
    return mRegion.size() / sizeof(ValueType);
  }

private:
  /// The read-only memory mapping of the file. Owns the lifetime of the bytes that every element
  /// pointer, reference, and iterator refers to, and supplies the byte length divided to compute
  /// size().
  const MmapRegion mRegion;
};

} // namespace nioc::containers
