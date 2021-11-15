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
    EXPECT_NO_THROW(StaticFrame<TestWorld>());
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


TEST(ParentFrame, StaticConstruction)
{
    EXPECT_NO_THROW(ParentFrame<TestWorld>());
}


TEST(ParentFrame, DynamicConstruction)
{
    EXPECT_NO_THROW(ParentFrame<DynamicFrame<>>("Test845"));
    EXPECT_NO_THROW(ParentFrame<DynamicFrame<>>(DynamicFrame("Test943")));
}


TEST(ParentFrame, DynamicName)
{
    ParentFrame<DynamicFrame<>> p1("Test845");
    ParentFrame<DynamicFrame<>> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.parentFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.parentFrame());
}


TEST(ChildFrame, StaticConstruction)
{
    EXPECT_NO_THROW(ChildFrame<TestWorld>());
}


TEST(ChildFrame, DynamicConstruction)
{
    EXPECT_NO_THROW(ChildFrame<DynamicFrame<>>("Test845"));
    EXPECT_NO_THROW(ChildFrame<DynamicFrame<>>(DynamicFrame("Test943")));
}


TEST(ChildFrame, DynamicName)
{
    ChildFrame<DynamicFrame<>> p1("Test845");
    ChildFrame<DynamicFrame<>> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.childFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.childFrame());
}


TEST(DynamicFrame, EqualityCheck)
{
    DynamicFrame<std::string> d1("Test125");

    {
        DynamicFrame<std::string> d2("Test125");
        EXPECT_TRUE(d1 == d2);
        EXPECT_FALSE(d1 != d2);
    }

    {
        DynamicFrame<std::string> d3("Test43");
        EXPECT_FALSE(d1 == d3);
        EXPECT_TRUE(d1 != d3);
    }

    {
        DynamicFrame<const char*> d4("Test125");
        EXPECT_TRUE(d1 == d4);
        EXPECT_FALSE(d1 != d4);
    }

    DynamicFrame<int> d5(7);
    {
        DynamicFrame<int> d6(7);
        EXPECT_TRUE(d5 == d6);
        EXPECT_FALSE(d5 != d6);
    }

    {
        DynamicFrame<double> d7(7.0);
        EXPECT_TRUE(d7 == d5);
        EXPECT_FALSE(d7 != d5);
    }
}


} // End of namespace naksh::messages.

#pragma clang diagnostic pop
