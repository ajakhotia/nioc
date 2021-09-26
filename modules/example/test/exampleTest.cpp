////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/example/example.hpp>
#include <gtest/gtest.h>

namespace naksh::example
{

TEST(ExampleTest, Construction)
{
    const Example example("TestExample");
    EXPECT_EQ(example.name(), "TestExample");
}


TEST(ExampleTest, CopyConstruction)
{
    const Example example("anotherExample");
    const auto copy = example;

    EXPECT_EQ(copy.name(), "anotherExample");
}


TEST(ExampleTest, MoveConstruction)
{
    Example e1("yetAnotherExample");

    // NOTE(Move semantics):    Variable e1 is consumed in the process of move-construction of e2. e2 steals
    //                          underlying std::string as it has an efficient move-copy and move-assignment.
    //                          As a result, e1.name() returns an empty string. This is why one should not
    //                          use an l-value after calling move on it.
    const auto e2 = std::move(e1);

    EXPECT_EQ(e2.name(), "yetAnotherExample");
    EXPECT_EQ(e1.name(), "");
}

} // End of namespace naksh::example.

#pragma clang diagnostic pop
