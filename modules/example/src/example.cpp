////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE:    Local headers(usually private) are referenced using relative path from the parent
//          directory of the source file and are included using double quotes ("").
//
//          examplePrivateHeader.hpp lives in the same directory as example.cpp, so it can be
//          included directly as below.
#include "examplePrivateHeader.hpp"

// NOTE:    Public Header or header in other locations are referenced using the relative path from
//          include-root of the appropriate module / library that owns that said header. The
//          include-root of a module / library is specified by the build system. The parameters to
//          the target_include_directories(...) directive in the appropriate CMakeLists.txt
//          dictates the include-root of the corresponding module / library.
//
//          example.hpp is a public header that declares the interface for the example library.
//          CMakeLists.txt in the example module specifies "include" directory as the include-root
//          of the example library. So we reference the example.hpp header using a path w.r.t to
//          that "include" directory as below.
#include <cassert>
#include <naksh/example/example.hpp>

namespace naksh::example
{

Example::Example(): mName()
{
    // PrivateExample type declared and defined in the private header examplePrivateHeader.hpp
    // is only available amongst its sibling source files. It cannot and should not be accessed in
    // any other modules.
    PrivateExample privateExample(7);
    assert(privateExample.value() == 7);
}


Example::Example(std::string name): mName(std::move(name)) {}


[[nodiscard]] const std::string& Example::name() const noexcept
{
    return mName;
}


} // namespace naksh::example
