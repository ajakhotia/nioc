////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "examplePrivateHeader.hpp"
#include <naksh/example/example.hpp>
#include <cassert>

namespace naksh::example
{

Example::Example(): mName()
{
    // PrivateExample type declared and defined in the private header examplePrivateHeader.hpp
    // is only available amongst its sibling source files. It cannot and should not be accessed in
    // any other modules.
    PrivateExample pe(7);
    assert(pe.value() == 7);
}


Example::Example(std::string name): mName(std::move(name))
{
}


[[nodiscard]] const std::string& Example::name() const noexcept
{
    return mName;
}


} // End of namespace naksh::example.