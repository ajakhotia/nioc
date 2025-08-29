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
/// @brief  SFINAE based helper type to determine if the InstanceType is template
///         specialization of the TemplateType. This declaration represents the
///         general case when an InstanceType is not actually a specialization of
///         TemplateType, hence, the inheritance from the std::false_type.
/// @tparam InstanceType    Type of the instance to be checked. Eg: std::vector<int>.
/// @tparam TemplateType    Type of the template. Eg: std::vector.
template<typename InstanceType, template<typename...> typename TemplateType>
struct IsSpecialization: public std::false_type
{
};


/// @brief  Specialization of @class IsSpecialization for the case when the
///         InstanceType is actually and instance of the TemplateType. Hence,
///         the inheritance from std::true_type.
/// @tparam TemplateType    The template type, Eg: std::vector
/// @tparam Args            ArgumentsType that specialize the template type.
///                         Eg: <int, ...> for the case of std::vector<int>.
///
///                         The ... is for other parameter that may be needed
///                         by the template, such as allocator type in case of
///                         std::vector<>.
template<template<typename...> typename TemplateType, typename... Args>
struct IsSpecialization<TemplateType<Args...>, TemplateType>: public std::true_type
{
};


/// @brief  A helper to variable check semantics for @class IsSpecialization.
/// @tparam InstanceType    Type of the instance. Eg: std::vector<int>
/// @tparam TemplateType    Type of the template. Eg: std::vector.
template<typename InstanceType, template<typename...> typename TemplateType>
inline constexpr bool isSpecialization = IsSpecialization<InstanceType, TemplateType>::value;


/// @brief  Function to get a human readable name of a type.
///         This function uses boost ctti to interpret build the name string_view but
///         omits the trailing ']' character at the end.
/// @tparam Type    Type who's name needs to be acquired.
/// @return A compile-time interpretable std::string_view representing the name of
///         the type.
template<typename Type>
constexpr std::string_view prettyName() noexcept
{
    const auto name = std::string_view(boost::typeindex::ctti_type_index::type_id<Type>().name());
    return name.substr(0, name.size() - 1);
}

} // namespace nioc::common
