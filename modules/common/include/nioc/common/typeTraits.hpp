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
/// @brief Checks if a type is a template specialization.
///
/// Example: `IsSpecialization<std::vector<int>, std::vector>::value` is true.
///
/// @tparam InstanceType Type to check (e.g., std::vector<int>).
/// @tparam TemplateType Template to match (e.g., std::vector).
template<typename InstanceType, template<typename...> typename TemplateType>
struct IsSpecialization: public std::false_type
{
};

/// @brief Specialization for matching templates.
template<template<typename...> typename TemplateType, typename... Args>
struct IsSpecialization<TemplateType<Args...>, TemplateType>: public std::true_type
{
};

/// @brief Helper variable for IsSpecialization.
///
/// Example: `isSpecialization<std::vector<int>, std::vector>` is true.
template<typename InstanceType, template<typename...> typename TemplateType>
inline constexpr bool isSpecialization = IsSpecialization<InstanceType, TemplateType>::value;

/// @brief Gets a readable name for a type.
/// @tparam Type Type to get name for.
/// @return String view of the type name.
template<typename Type>
constexpr std::string_view prettyName() noexcept
{
  const auto name = std::string_view(boost::typeindex::ctti_type_index::type_id<Type>().name());
  return name.substr(0, name.size() - 1);
}

} // namespace nioc::common
