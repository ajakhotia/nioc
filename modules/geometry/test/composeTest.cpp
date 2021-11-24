////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/geometry/compose.hpp>
#include <gtest/gtest.h>

namespace naksh::geometry
{

class Sun;
class AlphaCenturi;
class Uranus;
class Pluto;


//TEST(FramesEqual, StaticLhsStaticRhs)
//{
//    using MercuryMercury = FramesEqual<StaticFrame<Mercury>, StaticFrame<Mercury>>;
//    EXPECT_TRUE(MercuryMercury::kStaticallyEqual);
//
//    // Ill-legal code - won't compile.
//    //using MercuryVenus = FramesEqual<StaticFrame<Mercury>, StaticFrame<Venus>>;
//    //EXPECT_FALSE(MercuryVenus::kStaticEqual);
//}
//
//
//TEST(FramesEqual, StaticLhsDynamicRhs)
//{
//    using MercuryDynamic = FramesEqual<StaticFrame<Mercury>, DynamicFrame>;
//
//    MercuryDynamic md1(DynamicFrame("naksh::geometry::Mercury"));
//    EXPECT_TRUE(md1.value());
//
//    MercuryDynamic md2(DynamicFrame("DynamicTortoise"));
//    EXPECT_FALSE(md2.value());
//}
//
//
//TEST(FramesEqual, DynamicLhsStaticRhs)
//{
//    using DynamicVenus = FramesEqual<DynamicFrame, StaticFrame<Venus>>;
//
//    DynamicVenus dv1(DynamicFrame("naksh::geometry::Venus"));
//    EXPECT_TRUE(dv1.value());
//
//    DynamicVenus dv2(DynamicFrame("DynamicSnail"));
//    EXPECT_FALSE(dv2.value());
//}
//
//
//TEST(FramesEqual, DynamicLhsDynamicRhs)
//{
//    using DynamicDynamic = FramesEqual<DynamicFrame, DynamicFrame>;
//
//    DynamicDynamic dd1(DynamicFrame("Neptune"), DynamicFrame("Neptune"));
//    EXPECT_TRUE(dd1.value());
//
//    DynamicDynamic dd2(DynamicFrame("Neptune"), DynamicFrame("Pluto"));
//    EXPECT_FALSE(dd2.value());
//}



TEST(ComposeTransform, StaticStaticStaticStatic)
{
    Transform<StaticFrame<Sun>, StaticFrame<Uranus>> lhs;
    Transform<StaticFrame<Uranus>, StaticFrame<Pluto>> rhs;

    const auto result = composeTransform(lhs, rhs);
    static_assert(std::is_same_v<typename decltype(result)::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename decltype(result)::ChildFrame, StaticFrame<Pluto>>);
}


TEST(ComposeTransform, StaticStaticStaticDynamic)
{
    Transform<StaticFrame<Sun>, StaticFrame<Uranus>> lhs;
    Transform<StaticFrame<Uranus>, DynamicFrame> rhs("milkyWay");

    const auto result = composeTransform(lhs, rhs);
    static_assert(std::is_same_v<typename decltype(result)::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename decltype(result)::ChildFrame, DynamicFrame>);

    EXPECT_EQ(result.childFrame().name(), "milkyWay");
}


} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
