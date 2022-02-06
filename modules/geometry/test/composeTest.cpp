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


TEST(ComposeTransform, StaticStaticDynamicStatic)
{
    using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
    using DynamicFromPlutoFrames = FrameReferences<DynamicFrame, StaticFrame<Pluto>>;

    {
        SunFromUranusFrames lhs;
        DynamicFromPlutoFrames rhs("naksh::geometry::Uranus");

        EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
        const auto result = composeFrameReferences(lhs, rhs);

        using ResultType = decltype(result);
        static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
    }

    {
        SunFromUranusFrames lhs;
        DynamicFromPlutoFrames rhs("milkyWay");

        EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
    }
}


TEST(ComposeTransform, StaticDynamicStaticStatic)
{
    using SunFromDynamicFrames = FrameReferences<StaticFrame<Sun>, DynamicFrame>;
    using DynamicFromPlutoFrames = FrameReferences<StaticFrame<Uranus>, StaticFrame<Pluto>>;

    {
        SunFromDynamicFrames lhs("naksh::geometry::Uranus");
        DynamicFromPlutoFrames rhs;

        EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
        const auto result = composeFrameReferences(lhs, rhs);

        using ResultType = decltype(result);
        static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
    }

    {
        SunFromDynamicFrames lhs("milkyWay");
        DynamicFromPlutoFrames rhs;

        EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
    }
}


TEST(ComposeTransform, StaticDynamicDynamicStatic)
{
    using SunFromDynamicFrames = FrameReferences<StaticFrame<Sun>, DynamicFrame>;
    using DynamicFromPlutoFrames = FrameReferences<DynamicFrame, StaticFrame<Pluto>>;

    {
        SunFromDynamicFrames lhs("naksh::geometry::Uranus");
        DynamicFromPlutoFrames rhs("naksh::geometry::Uranus");

        EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
        const auto result = composeFrameReferences(lhs, rhs);

        using ResultType = decltype(result);
        static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
    }

    {
        SunFromDynamicFrames lhs("MilkyWay");
        DynamicFromPlutoFrames rhs("Andromeda");

        EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
    }
}


TEST(ComposeTransform, DynamicStaticStaticDynamic)
{
    using DynamicFromUranusFrames = FrameReferences<DynamicFrame, StaticFrame<Uranus>>;
    using UranusFromDynamicFrames = FrameReferences<StaticFrame<Uranus>, DynamicFrame>;

    {
        DynamicFromUranusFrames lhs("naksh::geometry::Sun");
        UranusFromDynamicFrames rhs("naksh::geometry::Pluto");

        EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
        const auto result = composeFrameReferences(lhs, rhs);

        using ResultType = decltype(result);
        static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
        EXPECT_EQ(result.parentFrame().name(), "naksh::geometry::Sun");
        EXPECT_EQ(result.childFrame().name(), "naksh::geometry::Pluto");
    }
}

} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
