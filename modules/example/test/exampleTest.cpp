////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/example/example.hpp>

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
    Example example1("yetAnotherExample");

    // NOTE(Move semantics):    Variable e1 is consumed in the process of move-construction of e2.
    //                          e2 steals underlying std::string as it has an efficient move-copy
    //                          and move-assignment. As a result, e1.name() returns an empty string.
    //                          This is why one should not use an l-value after calling move on it.
    const auto example2 = std::move(example1);

    EXPECT_EQ(example2.name(), "yetAnotherExample");
    EXPECT_EQ(example1.name(), ""); // NOLINT(clang-analyzer-cplusplus.Move)
}

} // namespace naksh::example

#pragma clang diagnostic pop
