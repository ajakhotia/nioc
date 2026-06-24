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

/// @brief A read-only, memory-mapped array of @p ValueType over an existing file.
///
/// Maps an existing file read-only; its size determines the element count. The elements form a
/// contiguous range whose address is stable for the object's lifetime, so a pointer or iterator
/// into it stays valid until the object is destroyed. A contiguous range, so it converts to a
/// `std::span` and works with the standard algorithms. The backing storage is read-only: a stray
/// write is a hardware fault, not a silent corruption.
///
/// Allocate it on the heap and share it through a pointer: the type is neither copyable nor
/// movable, so a pointer or iterator never outlives its mapping.
///
/// @tparam ValueType Element type: a non-const, non-volatile, trivially copyable type (its bytes
/// are its whole value). The array is read-only because it is an @ref MmapConstArray, not because
/// @p ValueType is const.
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
  using const_iterator = const_pointer;

  /// @brief Maps the existing file at @p path read-only.
  ///
  /// @param path File to map; must exist and its length must be a whole multiple of
  /// `sizeof(ValueType)`.
  ///
  /// @throws std::runtime_error If the file cannot be opened or mapped, or its length is not a
  /// whole number of elements.
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

  /// @brief Returns a pointer to the first element.
  [[nodiscard]] const_pointer data() const noexcept
  {
    return asElementPointer<ValueType>(mRegion.bytes());
  }

  /// @brief Returns a reference to the element at @p index.
  ///
  /// @param index Element position, less than @ref size. Out-of-range access is undefined.
  [[nodiscard]] const_reference operator[](const size_type index) const noexcept
  {
    return data()[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Returns an iterator to the first element.
  [[nodiscard]] const_iterator begin() const noexcept
  {
    return data();
  }

  /// @brief Returns an iterator one past the last element.
  [[nodiscard]] const_iterator end() const noexcept
  {
    return data() + size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Returns a const iterator to the first element.
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// @brief Returns a const iterator one past the last element.
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// @brief Returns whether the array holds no elements.
  [[nodiscard]] bool empty() const noexcept
  {
    return mRegion.empty();
  }

  /// @brief Returns the number of elements.
  [[nodiscard]] size_type size() const noexcept
  {
    return mRegion.size() / sizeof(ValueType);
  }

private:
  const MmapRegion mRegion;
};

} // namespace nioc::containers
