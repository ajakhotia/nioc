////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "mmapRegion.hpp"
#include <cstddef>
#include <filesystem>
#include <type_traits>
#include <utility>

namespace nioc::containers
{

/// @brief A fixed-size, writable, memory-mapped array of @p ValueType.
///
/// Creates its file at a fixed element count and maps it read-write. The elements form a contiguous
/// range whose address is stable for the object's lifetime, so a pointer or iterator into it stays
/// valid until the object is destroyed. Read through a const array, write through a non-const one.
/// A contiguous range, so it converts to a `std::span` and works with the standard algorithms.
///
/// Allocate it on the heap and share it through a pointer: the type is neither copyable nor
/// movable, so a pointer or iterator never outlives its mapping.
///
/// @tparam ValueType Element type: a non-const, non-volatile, trivially copyable type (its bytes
/// are its whole value). Constness is the container's job — use @ref MmapConstArray for read-only.
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

  /// @brief Creates @p path holding @p count elements and maps it for writing.
  ///
  /// @param path File to create; its contents are discarded if it already exists. Parent
  /// directories are created as needed.
  ///
  /// @param count Number of elements.
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

  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return asElementPointer<ValueType>(self.mRegion.bytes());
  }

  [[nodiscard]] decltype(auto) operator[](this auto&& self, const size_type index) noexcept
  {
    return self.data()[index];
  }

  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.data();
  }

  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.data() + self.size();
  }

  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return mRegion.empty();
  }

  [[nodiscard]] size_type size() const noexcept
  {
    return mRegion.size() / sizeof(ValueType);
  }

  /// @brief Resizes the backing file to @p count elements.
  ///
  /// Drops the elements past @p count, the way resizing a `std::vector` smaller would. The mapping
  /// is unchanged, so accessing elements at or beyond @p count afterward is undefined; call this
  /// only once those elements are done with.
  ///
  /// @param count Number of leading elements to keep; must not exceed @ref size.
  void resize(const size_type count) noexcept
  {
    mRegion.resize(count * sizeof(ValueType));
  }

private:
  MmapRegion mRegion;
};

} // namespace nioc::containers
