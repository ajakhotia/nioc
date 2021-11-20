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

class Mercury;
class Venus;


TEST(StaticFrame, construction)
{
    // A static frame cannot be constructed as one should never need to.
    //EXPECT_NO_THROW(StaticFrame<Mercury>());
}


TEST(StaticFrame, name)
{
    static_assert("naksh::geometry::Mercury" == StaticFrame<Mercury>::name());
    EXPECT_EQ("naksh::geometry::Venus", StaticFrame<Venus>::name());
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


TEST(FramesEqual, StaticLhsStaticRhs)
{
    using MercuryMercury = FramesEqual<StaticFrame<Mercury>, StaticFrame<Mercury>>;
    static_assert(MercuryMercury::value());
    EXPECT_TRUE(MercuryMercury::value());
}


TEST(FramesEqual, StaticLhsDynamicRhs)
{
    using MercuryDynamic = FramesEqual<StaticFrame<Mercury>, DynamicFrame>;

    MercuryDynamic md1(DynamicFrame("naksh::geometry::Mercury"));
    EXPECT_TRUE(md1.value());

    MercuryDynamic md2(DynamicFrame("DynamicTortoise"));
    EXPECT_FALSE(md2.value());
}


TEST(FramesEqual, DynamicLhsStaticRhs)
{
    using DynamicVenus = FramesEqual<DynamicFrame, StaticFrame<Venus>>;

    DynamicVenus dv1(DynamicFrame("naksh::geometry::Venus"));
    EXPECT_TRUE(dv1.value());

    DynamicVenus dv2(DynamicFrame("DynamicSnail"));
    EXPECT_FALSE(dv2.value());
}


TEST(FramesEqual, DynamicLhsDynamicRhs)
{
    using DynamicDynamic = FramesEqual<DynamicFrame, DynamicFrame>;

    DynamicDynamic dd1(DynamicFrame("Neptune"), DynamicFrame("Neptune"));
    EXPECT_TRUE(dd1.value());

    DynamicDynamic dd2(DynamicFrame("Neptune"), DynamicFrame("Pluto"));
    EXPECT_FALSE(dd2.value());
}


} // End of namespace naksh::messages.

#pragma clang diagnostic pop
