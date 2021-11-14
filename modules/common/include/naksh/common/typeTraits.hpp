////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/type_index/ctti_type_index.hpp>
#include <type_traits>
#include <string_view>


namespace naksh::common
{

template<typename InstanceType, template <typename ...> typename TemplateType>
struct IsSpecialization : public std::false_type
{
};


template< template<typename ...> typename Template, typename ...Args>
struct IsSpecialization<Template<Args...>, Template> : public std::true_type
{
};


/// @brief  Function to get a human readable name of a type.
///         This function uses boost ctti to interpret build the name string_view but
///         omits the trailing ']' character at the end.
/// @tparam Type    Type who's name needs to be acquired.
/// @return A compile-time interpretable std::string_view representing the name of
///         the type.
template<typename Type>
constexpr std::string_view prettyName() noexcept
{
    const auto name = std::string_view(
            boost::typeindex::ctti_type_index::type_id<Type>().name());

    return name.substr(0, name.size() - 1);
}

} // End of namespace naksh::common.
