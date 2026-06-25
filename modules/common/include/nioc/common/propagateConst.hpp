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

/// @brief A pointer wrapper that makes the wrapper's own constness carry through to the pointee.
///
/// Raw and smart pointers do not do this: a `const` pointer still hands you a mutable pointee.
/// Wrap the pointer in `PropagateConst` and the rule flips. Reach through a non-const wrapper and
/// you get a mutable pointee; reach through a const wrapper and you get a `const` pointee. Use it
/// for a member pointer whose pointee should be read-only in const member functions. Models a
/// minimal subset of `std::experimental::propagate_const`.
///
/// Example:
///
///     class Widget
///     {
///       PropagateConst<std::unique_ptr<Impl>> mImpl;
///
///       void touch()
///       {
///         mImpl->mutate(); // non-const member: mImpl-> yields a mutable Impl*
///       }
///
///       void inspect() const
///       {
///         mImpl->read(); // const member: mImpl-> yields a const Impl*
///       }
///     };
///
/// @tparam Pointer The wrapped handle, e.g. `T*`, `std::unique_ptr<T>`, or `std::shared_ptr<T>`.
/// Stored by value; it owns and manages the pointee exactly as it would on its own. Must be
/// dereferenceable with `operator*` and usable with `std::to_address`.
///
/// @see std::experimental::propagate_const
template<typename Pointer>
class PropagateConst
{
public:
  /// The pointed-to type. Deduced from dereferencing `Pointer`, with any reference stripped.
  using element_type = std::remove_reference_t<decltype(*std::declval<Pointer&>())>;

  /// @brief Construct an empty wrapper holding a value-initialized (typically null) `Pointer`.
  PropagateConst() = default;

  /// @brief Take ownership of @p pointer by moving it into the wrapper.
  ///
  /// @param pointer The handle to wrap. Moved from, so its prior owner is left empty.
  explicit PropagateConst(Pointer pointer) noexcept: mPointer{std::move(pointer)} {}

  /// @brief Return the underlying raw pointer, mutable.
  ///
  /// @return A non-owning `element_type*`; null if the wrapper is empty.
  [[nodiscard]] element_type* get() noexcept
  {
    return std::to_address(mPointer);
  }

  /// @brief Return the underlying raw pointer as a pointer-to-const.
  ///
  /// @return A non-owning `const element_type*`; null if the wrapper is empty.
  [[nodiscard]] const element_type* get() const noexcept
  {
    return std::to_address(mPointer);
  }

  /// @brief Access the pointee, mutable.
  ///
  /// Undefined if the wrapper is empty.
  [[nodiscard]] element_type& operator*() noexcept
  {
    return *mPointer;
  }

  /// @brief Access the pointee as `const`.
  ///
  /// Undefined if the wrapper is empty.
  [[nodiscard]] const element_type& operator*() const noexcept
  {
    return *mPointer;
  }

  /// @brief Access the pointee's members through a mutable pointer.
  ///
  /// Returns null if the wrapper is empty.
  [[nodiscard]] element_type* operator->() noexcept
  {
    return get();
  }

  /// @brief Access the pointee's members through a pointer-to-const.
  ///
  /// Returns null if the wrapper is empty.
  [[nodiscard]] const element_type* operator->() const noexcept
  {
    return get();
  }

  /// @brief Report whether the wrapper holds a non-null pointer.
  [[nodiscard]] explicit operator bool() const noexcept
  {
    return static_cast<bool>(mPointer);
  }

private:
  /// The wrapped handle. Owns and manages the pointee; value-initialized (typically null) by
  /// default. All accessors read through it, applying their own constness to the pointee.
  Pointer mPointer{};
};

} // namespace nioc::common
