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
/// @brief True if @p InstanceType is a specialization of @p TemplateType.
///
/// Example: `IsSpecialization<std::vector<int>, std::vector>::value` is true.
///
/// @tparam InstanceType The type to check (e.g. std::vector<int>).
/// @tparam TemplateType The template to match against (e.g. std::vector).
template<typename InstanceType, template<typename...> typename TemplateType>
struct IsSpecialization: public std::false_type
{
};

/// @brief Match: @p InstanceType is a specialization of @p TemplateType.
template<template<typename...> typename TemplateType, typename... Args>
struct IsSpecialization<TemplateType<Args...>, TemplateType>: public std::true_type
{
};

/// @brief Variable form of @ref IsSpecialization.
///
/// Example: `isSpecialization<std::vector<int>, std::vector>` is true.
template<typename InstanceType, template<typename...> typename TemplateType>
inline constexpr bool isSpecialization = IsSpecialization<InstanceType, TemplateType>::value;

/// @brief Returns the fully qualified, readable name of @p Type at compile time.
///
/// Example: `prettyName<nioc::common::SignalCatcher>()` is `"nioc::common::SignalCatcher"`.
template<typename Type>
consteval std::string_view prettyName() noexcept
{
  const auto name = std::string_view(boost::typeindex::ctti_type_index::type_id<Type>().name());
  return name.substr(0, name.size() - 1);
}

} // namespace nioc::common
