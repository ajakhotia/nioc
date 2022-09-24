////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "examplePrivateHeader.hpp"
#include <cassert>
#include <nioc/example/example.hpp>

auto main() -> int
{
    const nioc::example::Example myExample("Yolo");
    assert("Yolo" == myExample.name());

    const nioc::example::PrivateExample myPrivateExample(12);
    assert(myPrivateExample.value() == 12);

    return EXIT_SUCCESS;
}
