////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE:    Local headers(usually private) are referenced using relative path from the parent directory of the
//          source file and are included using double quotes ("")
#include "examplePrivateHeader.hpp"


// NOTE:    Public Header or header in other locations are referenced using the relative path from include-root
//          of the appropriate module / library. The include-root of a module / library is specified by the
//          build system. The target_include_directories(...) directive in appropriate CMakeLists.txt dictates
//          the include root of the corresponding module / library.
#include <naksh/example/example.hpp>

#include <cassert>

auto main() -> int
{
    const naksh::example::Example myExample("Yolo");
    assert("Yolo" == myExample.name());

    const naksh::example::PrivateExample myPrivateExample(12);
    assert(myPrivateExample.value() == 12);

    return EXIT_SUCCESS;
}