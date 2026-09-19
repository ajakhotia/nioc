////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/type_index/ctti_type_index.hpp>
#include <cstddef>
#include <new>
#include <span>
#include <string_view>
#include <type_traits>

namespace nioc::common
{
/// @brief Trait that reports whether @p InstanceType is some specialization of the class template
/// @p TemplateType. Derives from `std::true_type` on a match, `std::false_type` otherwise.
///
/// Example:
///
///     IsSpecialization<std::vector<int>, std::vector>::value  // true
///     IsSpecialization<int, std::vector>::value               // false
///
/// @tparam InstanceType The type to test. Top-level `const` or reference qualifiers defeat the
/// match, so strip them first if needed.
///
/// @tparam TemplateType The template to match against. Pass it bare, not as an instantiated type
/// (e.g. `std::vector`, not `std::vector<int>`). Only templates whose parameters are all types are
/// supported; templates with non-type or template-template parameters never match.
///
/// @see isSpecialization
template<typename InstanceType, template<typename...> typename TemplateType>
struct IsSpecialization: public std::false_type
{
};

/// @brief Matching specialization of @ref IsSpecialization: chosen when the instance type really is
/// `TemplateType<Args...>`, yielding the `std::true_type` result.
template<template<typename...> typename TemplateType, typename... Args>
struct IsSpecialization<TemplateType<Args...>, TemplateType>: public std::true_type
{
};

/// @brief Value form of @ref IsSpecialization: `true` when @p InstanceType is a specialization of
/// @p TemplateType, else `false`. Use this in place of the trait's `::value` member.
///
/// @see IsSpecialization
template<typename InstanceType, template<typename...> typename TemplateType>
inline constexpr bool isSpecialization = IsSpecialization<InstanceType, TemplateType>::value;

/// @brief Return a human-readable name for @p Type, computed at compile time.
///
/// Example:
///
///     prettyName<std::vector<int>>()  // e.g. "std::vector<int, std::allocator<int>>"
///
/// @tparam Type The type to name. Must be supplied explicitly; it cannot be deduced.
///
/// @return A view of the compiler's textual type name. The view refers to static storage that
/// lives for the whole program. The exact spelling is compiler-dependent; use it for diagnostics,
/// not as a stable identifier or map key.
template<typename Type>
consteval std::string_view prettyName() noexcept
{
  return std::string_view(boost::typeindex::ctti_type_index::type_id<Type>().name());
}

/// @brief Whether @p Type is an implicit-lifetime type ([basic.types.general], [class.prop]):
/// objects of it may be created implicitly in suitable storage, so a pointer to such storage may
/// be treated as a pointer to a live @p Type without construction. Scalars, arrays, aggregates,
/// and classes with a trivial destructor and at least one trivial constructor (default, copy, or
/// move) qualify. One approximation: the standard excludes an aggregate whose destructor is
/// user-provided, which no type trait can observe, so such an aggregate is accepted here.
template<typename Type>
inline constexpr bool isImplicitLifetime = std::is_scalar_v<Type> or
                                           std::is_array_v<Type> or
                                           std::is_aggregate_v<Type> or
                                           (std::is_class_v<Type> and
                                            std::is_trivially_destructible_v<Type> and
                                            (std::is_trivially_default_constructible_v<Type> or
                                             std::is_trivially_copy_constructible_v<Type> or
                                             std::is_trivially_move_constructible_v<Type>));

/// @brief Treat the bytes at @p bytes as a live @p Type, in the sense of C++23's
/// std::start_lifetime_as, which no shipped standard library implements yet. For an array, point
/// at its first element; the result indexes as the array.
///
/// Bytes that arrive from outside the program (a memory-mapped file, a kernel-shared ring, a
/// buffer filled by a syscall) already hold implicitly created objects of any implicit-lifetime
/// type; std::launder yields a pointer to them without the compiler assuming the bytes are
/// unrelated. Alignment and size are the caller's responsibility, as they are for the standard
/// facility.
///
/// Example:
///
///     auto* header = startLifetimeAs<Header>(region.data());
///
/// @tparam Type The type to view the bytes as. Must be an implicit-lifetime type.
///
/// @param bytes The object's first byte; suitably aligned for @p Type and the start of at least
/// sizeof(Type) bytes.
///
/// @return A pointer to the object.
template<typename Type>
  requires isImplicitLifetime<Type>
[[nodiscard]] Type* startLifetimeAs(std::byte* const bytes) noexcept
{
  // This function is the one sanctioned home of the byte-to-object reinterpretation.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return std::launder(reinterpret_cast<Type*>(bytes));
}

/// @brief Read-only overload of startLifetimeAs for const bytes.
template<typename Type>
  requires isImplicitLifetime<Type>
[[nodiscard]] const Type* startLifetimeAs(const std::byte* const bytes) noexcept
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return std::launder(reinterpret_cast<const Type*>(bytes));
}

/// @brief Treat @p bytes as a contiguous array of live @p Type objects, in the sense of C++23's
/// std::start_lifetime_as_array: the array holds `bytes.size() / sizeof(Type)` elements, and
/// trailing bytes that do not fill an element are excluded. Const bytes yield const elements.
///
/// @tparam Type The element type. Must be an implicit-lifetime type.
///
/// @tparam Byte Deduced: `std::byte` or `const std::byte`.
///
/// @tparam Extent Deduced: the span's extent, static or dynamic.
///
/// @param bytes The array's bytes; suitably aligned for @p Type.
///
/// @return A dynamic-extent span over the elements.
///
/// @see startLifetimeAs
template<typename Type, typename Byte, std::size_t Extent>
  requires isImplicitLifetime<Type> and std::is_same_v<std::remove_const_t<Byte>, std::byte>
[[nodiscard]] auto startLifetimeAsArray(const std::span<Byte, Extent> bytes) noexcept
{
  using Element = std::conditional_t<std::is_const_v<Byte>, const Type, Type>;
  return std::span<Element>{startLifetimeAs<Type>(bytes.data()), bytes.size() / sizeof(Type)};
}

} // namespace nioc::common
