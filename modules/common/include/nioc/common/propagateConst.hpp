////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace nioc::common
{

/// @brief A pointer wrapper that propagates the enclosing object's const-ness to the pointee.
///
/// A raw or smart pointer member does not carry const through to what it points at: in a const
/// member function the handle is const but the pointee stays mutable, so a const object can hand
/// out mutable access. Hold the pointer in a PropagateConst instead — const access yields a const
/// pointee, non-const access a mutable one — and deep const is enforced by the type rather than by
/// remembering to write (and correctly qualify) a const/non-const accessor pair.
///
/// A drop-in, dependency-free stand-in for `std::experimental::propagate_const`. Pairs well with a
/// C++23 deducing-this accessor: `auto data(this auto&& self) { return self.mPtr.get(); }` is one
/// function that is correctly const on both paths.
///
/// @tparam Pointer A pointer type: raw (e.g. `T*`) or smart (e.g. `std::unique_ptr<T>`).
template<typename Pointer>
class PropagateConst
{
public:
  using element_type = std::remove_reference_t<decltype(*std::declval<Pointer&>())>;

  PropagateConst() = default;

  /// @brief Wraps @p pointer.
  explicit PropagateConst(Pointer pointer) noexcept: mPointer{std::move(pointer)} {}

  [[nodiscard]] element_type* get() noexcept
  {
    return std::to_address(mPointer);
  }

  [[nodiscard]] const element_type* get() const noexcept
  {
    return std::to_address(mPointer);
  }

  [[nodiscard]] element_type& operator*() noexcept
  {
    return *mPointer;
  }

  [[nodiscard]] const element_type& operator*() const noexcept
  {
    return *mPointer;
  }

  [[nodiscard]] element_type* operator->() noexcept
  {
    return get();
  }

  [[nodiscard]] const element_type* operator->() const noexcept
  {
    return get();
  }

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return static_cast<bool>(mPointer);
  }

private:
  Pointer mPointer{};
};

} // namespace nioc::common
