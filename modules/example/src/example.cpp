////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "examplePrivateHeader.hpp"
#include <naksh/example/example.hpp>

namespace naksh::example
{

Example::Example(): mName()
{
}


Example::Example(std::string name): mName(std::move(name))
{
}


[[nodiscard]] const std::string& Example::name() const noexcept
{
    return mName;
}


} // End of namespace naksh::example.