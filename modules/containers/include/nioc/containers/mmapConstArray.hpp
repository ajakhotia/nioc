////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "mmapRegion.hpp"
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <nioc/common/exception.hpp>
#include <span>
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
/// Non-copyable; move-constructible. The mapped bytes never change address, so a move transfers
/// the mapping while every pointer, reference, and iterator stays valid until the owning
/// container is destroyed; the moved-from view is empty. Concurrent reads are safe; behaviour is
/// unspecified if the file is changed through another handle while it is mapped.
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
  using const_reference = const ValueType&;
  using const_pointer = const ValueType*;

  /// Contiguous, random-access iterator over the elements.
  using const_iterator = std::span<const ValueType>::iterator;

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

  /// @brief Take over @p other's mapping and file descriptor, leaving @p other empty and fit only
  /// for destruction.
  MmapConstArray(MmapConstArray&&) noexcept = default;

  ~MmapConstArray() = default;

  MmapConstArray& operator=(const MmapConstArray&) = delete;

  MmapConstArray& operator=(MmapConstArray&&) noexcept = delete;

  /// @brief Pointer to the first element. Equals end() when empty. Valid for the array's lifetime.
  [[nodiscard]] const_pointer data() const noexcept
  {
    return elements().data();
  }

  /// @brief Reference to the element at @p index.
  ///
  /// @param index Element position. Not bounds-checked; must be less than size().
  [[nodiscard]] const_reference operator[](const size_type index) const noexcept
  {
    return *std::next(begin(), static_cast<difference_type>(index));
  }

  /// @brief Reference to the element at @p index, bounds-checked.
  ///
  /// @param index Element position.
  ///
  /// @throws std::out_of_range if @p index is not less than size().
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

  /// @brief Iterator to the first element.
  [[nodiscard]] const_iterator begin() const noexcept
  {
    return elements().begin();
  }

  /// @brief Iterator one past the last element.
  [[nodiscard]] const_iterator end() const noexcept
  {
    return elements().end();
  }

  /// @brief Const iterator to the first element; same as begin().
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// @brief Const iterator one past the last element; same as end().
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// @brief True if the array holds no elements.
  [[nodiscard]] bool empty() const noexcept
  {
    return mRegion.empty();
  }

  /// @brief Number of elements. Equals the file's byte length divided by sizeof(ValueType).
  [[nodiscard]] size_type size() const noexcept
  {
    return mRegion.size() / sizeof(ValueType);
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
  /// @see MmapRegion::evict
  template<std::contiguous_iterator Iterator>
    requires std::is_same_v<std::iter_value_t<Iterator>, ValueType>
  void evict(const Iterator first, const Iterator last) const noexcept
  {
    mRegion.evict(std::as_bytes(std::span{first, last}));
  }

private:
  /// The elements as one contiguous span over the mapping.
  [[nodiscard]] std::span<const ValueType> elements() const noexcept
  {
    return mRegion.elements<ValueType>();
  }

  /// The read-only memory mapping of the file. Owns the lifetime of the bytes that every element
  /// pointer, reference, and iterator refers to, and supplies the byte length divided to compute
  /// size().
  MmapRegion mRegion;
};

} // namespace nioc::containers
