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
class AlphaCentauri;
class Uranus;
class Pluto;

namespace helpers
{

TEST(assertFrameEqual, StaticLhsStaticRhs)
{
    EXPECT_NO_THROW((
        assertFrameEqual<StaticFrame<Sun>, StaticFrame<Sun>>()
        ));

    // Ill-legal code - won't compile.
    // helpers::assertFrameEqual<StaticFrame<Sun>, StaticFrame<AlphaCentauri>>();
}


TEST(assertFrameEqual, StaticLhsDynamicRhs)
{
    EXPECT_NO_THROW(
        (assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW(
        (assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("Bloop"))),
        TransformCompositionException);
}


TEST(assertFrameEqual, DynamicLhsStaticRhs)
{
    EXPECT_NO_THROW(
        (assertFrameEqual<DynamicFrame, StaticFrame<Sun>>(DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW(
        (assertFrameEqual<DynamicFrame, StaticFrame<AlphaCentauri>>(DynamicFrame("Bloop"))),
        TransformCompositionException);
}


TEST(assertFrameEqual, DynamicLhsDynamicRhs)
{
    EXPECT_NO_THROW(
        (assertFrameEqual<DynamicFrame, DynamicFrame>(
            DynamicFrame("naksh::geometry::Sun"),
            DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW(
        (assertFrameEqual<DynamicFrame, DynamicFrame>(
            DynamicFrame("Bloop"),
            DynamicFrame("Bleep"))),
        TransformCompositionException);
}

} // End of namespace helpers.


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
