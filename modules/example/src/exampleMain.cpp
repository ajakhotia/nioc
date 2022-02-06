////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "examplePrivateHeader.hpp"

#include <cassert>
#include <naksh/example/example.hpp>

auto main() -> int
{
    const naksh::example::Example myExample("Yolo");
    assert("Yolo" == myExample.name());

    const naksh::example::PrivateExample myPrivateExample(12);
    assert(myPrivateExample.value() == 12);

    return EXIT_SUCCESS;
}
