////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/type_index/ctti_type_index.hpp>
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
/// @return A view of the compiler's textual type name, with the trailing end-of-signature
/// character (e.g. `]` on GCC/Clang) removed. The view refers to static storage that lives for the
/// whole program. The exact spelling is compiler-dependent; use it for diagnostics, not as a
/// stable identifier or map key.
template<typename Type>
consteval std::string_view prettyName() noexcept
{
  const auto name = std::string_view(boost::typeindex::ctti_type_index::type_id<Type>().name());
  return name.substr(0, name.size() - 1);
}

} // namespace nioc::common
