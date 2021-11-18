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

class Earth;
class Mars;


TEST(ParentConceptTmpl, StaticConstruction)
{
    EXPECT_NO_THROW(ParentConceptTmpl<Earth>());
}


TEST(ParentConceptTmpl, DynamicConstruction)
{
    EXPECT_NO_THROW(ParentConceptTmpl<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ParentConceptTmpl<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ParentConceptTmpl, DynamicName)
{
    ParentConceptTmpl<DynamicFrame> p1("Test845");
    ParentConceptTmpl<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.parentFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.parentFrame());
}


TEST(ChildConceptTmpl, StaticConstruction)
{
    EXPECT_NO_THROW(ChildConceptTmpl<Earth>());
}


TEST(ChildConceptTmpl, DynamicConstruction)
{
    EXPECT_NO_THROW(ChildConceptTmpl<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ChildConceptTmpl<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ChildConceptTmpl, DynamicName)
{
    ChildConceptTmpl<DynamicFrame> p1("Test845");
    ChildConceptTmpl<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.childFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.childFrame());
}


} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
