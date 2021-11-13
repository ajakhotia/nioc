////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : NakshOps                                                                                                 /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <type_traits>

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


} // End of namespace naksh::common.
