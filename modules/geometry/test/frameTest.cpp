////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/geometry/frame.hpp>
#include <boost/type_index.hpp>
#include <gtest/gtest.h>

namespace naksh::geometry
{
class TestWorld;
class TestCow;

TEST(StaticFrame, construction)
{
    //EXPECT_NO_THROW(StaticFrame<TestWorld>());
}


TEST(StaticFrame, name)
{
    static_assert("naksh::geometry::TestWorld" == StaticFrame<TestWorld>::name());
    EXPECT_EQ("naksh::geometry::TestCow", StaticFrame<TestCow>::name());
}


TEST(DynamicFrame, construction)
{
    EXPECT_NO_THROW(DynamicFrame("testFrame145"));
}


TEST(DynamicFrame, name)
{
    DynamicFrame df("testFrame145");
    EXPECT_EQ("testFrame145", df.name());
}


TEST(DynamicFrame, EqualityCheck)
{
    DynamicFrame d1("Test125");

    {
        DynamicFrame d2("Test125");
        EXPECT_TRUE(d1 == d2);
        EXPECT_FALSE(d1 != d2);
    }

    {
        DynamicFrame d3("Test43");
        EXPECT_FALSE(d1 == d3);
        EXPECT_TRUE(d1 != d3);
    }
}


} // End of namespace naksh::messages.

#pragma clang diagnostic pop
