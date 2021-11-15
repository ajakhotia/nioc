////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/geometry/frameReferences.hpp>
#include <gtest/gtest.h>

namespace naksh::geometry
{

class TestAnotherWorld;
class TestAnotherCow;


TEST(ParentFrame, StaticConstruction)
{
    EXPECT_NO_THROW(ParentFrame<TestAnotherWorld>());
}


TEST(ParentFrame, DynamicConstruction)
{
    EXPECT_NO_THROW(ParentFrame<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ParentFrame<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ParentFrame, DynamicName)
{
    ParentFrame<DynamicFrame> p1("Test845");
    ParentFrame<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.parentFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.parentFrame());
}


TEST(ChildFrame, StaticConstruction)
{
    EXPECT_NO_THROW(ChildFrame<TestAnotherWorld>());
}


TEST(ChildFrame, DynamicConstruction)
{
    EXPECT_NO_THROW(ChildFrame<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ChildFrame<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ChildFrame, DynamicName)
{
    ChildFrame<DynamicFrame> p1("Test845");
    ChildFrame<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.childFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.childFrame());
}


TEST(FrameReferences, Construction)
{
    using RefType = FrameReferences<TestAnotherCow, TestAnotherWorld>;

    static_assert(RefType::Parent::name() == "naksh::geometry::TestAnotherCow");
    EXPECT_EQ(RefType::Parent::name(), "naksh::geometry::TestAnotherCow");

    FrameReferences<TestAnotherCow, DynamicFrame> fr1("TestChild2");
    static_assert(RefType::Parent::name() == "naksh::geometry::TestAnotherCow");
    EXPECT_EQ(decltype(fr1)::Parent::name(), "naksh::geometry::TestAnotherCow");
    EXPECT_EQ(fr1.childFrame(), DynamicFrame("TestChild2"));
}


} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
