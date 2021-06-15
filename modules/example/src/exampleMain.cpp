////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/example/example.hpp>
#include <cassert>

auto main() -> int
{
    const naksh::example::Example myExample("Yolo");

    assert("Yolo" == myExample.name());
    return EXIT_SUCCESS;
}