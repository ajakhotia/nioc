////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/common/typeTraits.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <deque>

namespace naksh::common
{

TEST(TypeTraits, IsSpecialization)
{
    static_assert(IsSpecialization<std::vector<int>, std::vector>::value);
    static_assert(not(IsSpecialization<std::vector<int>, std::deque>::value));

    EXPECT_TRUE(bool(IsSpecialization<std::vector<int>, std::vector>::value));
    EXPECT_FALSE(bool(IsSpecialization<std::vector<int>, std::deque>::value));
}


class TestType;
class AnotherTestType;


TEST(TypeTraits, prettyName)
{
    static_assert("naksh::common::TestType" == prettyName<TestType>());
    static_assert("naksh::common::TestType" != prettyName<AnotherTestType>());
    EXPECT_EQ("naksh::common::TestType", prettyName<TestType>());
    EXPECT_NE("naksh::common::TestType", prettyName<AnotherTestType>());
}


} // End of namespace naksh::common.

#pragma clang diagnostic pop
