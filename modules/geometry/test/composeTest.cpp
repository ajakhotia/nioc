////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/geometry/compose.hpp>

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
    EXPECT_NO_THROW((assertFrameEqual<StaticFrame<Sun>, StaticFrame<Sun>>()));

    static_assert(noexcept(assertFrameEqual<StaticFrame<Sun>, StaticFrame<Sun>>));
}


TEST(assertFrameEqual, StaticLhsDynamicRhs)
{
    EXPECT_NO_THROW(
        (assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW((assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("Bloop"))),
                 FrameCompositionException);

    static_assert(not noexcept(assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame(""))));
}


TEST(assertFrameEqual, DynamicLhsStaticRhs)
{
    EXPECT_NO_THROW(
        (assertFrameEqual<DynamicFrame, StaticFrame<Sun>>(DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW(
        (assertFrameEqual<DynamicFrame, StaticFrame<AlphaCentauri>>(DynamicFrame("Bloop"))),
        FrameCompositionException);

    static_assert(not noexcept(assertFrameEqual<DynamicFrame, StaticFrame<Sun>>(DynamicFrame(""))));
}


TEST(assertFrameEqual, DynamicLhsDynamicRhs)
{
    EXPECT_NO_THROW((assertFrameEqual<DynamicFrame, DynamicFrame>(
        DynamicFrame("naksh::geometry::Sun"), DynamicFrame("naksh::geometry::Sun"))));

    EXPECT_THROW((assertFrameEqual<DynamicFrame, DynamicFrame>(DynamicFrame("Bloop"),
                                                               DynamicFrame("Bleep"))),
                 FrameCompositionException);

    static_assert(not noexcept(
        assertFrameEqual<DynamicFrame, DynamicFrame>(DynamicFrame(""), DynamicFrame(""))));
}

} // End of namespace helpers.


TEST(ComposeTransform, StaticStaticStaticStatic)
{
    using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
    using UranusFromPlutoFrames = FrameReferences<StaticFrame<Uranus>, StaticFrame<Pluto>>;

    SunFromUranusFrames lhs;
    UranusFromPlutoFrames rhs;

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
}


TEST(ComposeTransform, StaticStaticStaticDynamic)
{
    using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
    using UranusFromDynaimcFrames = FrameReferences<StaticFrame<Uranus>, DynamicFrame>;

    SunFromUranusFrames lhs;
    UranusFromDynaimcFrames rhs("milkyWay");

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
    EXPECT_EQ(result.childFrame().name(), "milkyWay");
}

} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
