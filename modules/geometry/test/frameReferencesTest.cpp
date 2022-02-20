////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/geometry/frameReferences.hpp>

namespace naksh::geometry
{
class AlphaCentauri;
class Jupiter;
class Pluto;
class Saturn;
class Sun;
class Uranus;


TEST(FrameReferences, StaticParentStaticChild)
{
    using SaturnFromJupiter = FrameReferences<StaticFrame<Saturn>, StaticFrame<Jupiter>>;
    EXPECT_NO_THROW(SaturnFromJupiter());

    static_assert(SaturnFromJupiter::ParentFrame::name() == "naksh::geometry::Saturn");
    static_assert(SaturnFromJupiter::ChildFrame::name() == "naksh::geometry::Jupiter");

    const SaturnFromJupiter saturnFromJupiter;
    EXPECT_EQ("naksh::geometry::Saturn", decltype(saturnFromJupiter)::ParentFrame::name());
    EXPECT_EQ("naksh::geometry::Jupiter", decltype(saturnFromJupiter)::ChildFrame::name());
}


TEST(FrameReferences, StaticParentDynamicChild)
{
    using SaturnFromDynamic = FrameReferences<StaticFrame<Saturn>, DynamicFrame>;
    EXPECT_NO_THROW(SaturnFromDynamic("DynamicSaturn"));

    {
        static_assert(SaturnFromDynamic::ParentFrame::name() == "naksh::geometry::Saturn");
        // static_assert(SaturnFromDynamic::ChildFrame::name() == "");
    }

    {
        const SaturnFromDynamic saturnFromDynamic("DynamicSaturn");
        EXPECT_EQ("naksh::geometry::Saturn", decltype(saturnFromDynamic)::ParentFrame::name());
        EXPECT_EQ("DynamicSaturn", saturnFromDynamic.childFrame().name());
    }

    {
        const SaturnFromDynamic saturnFromDynamic(DynamicFrame("DynamicSaturn"));
        EXPECT_EQ("naksh::geometry::Saturn", decltype(saturnFromDynamic)::ParentFrame::name());
        EXPECT_EQ("DynamicSaturn", saturnFromDynamic.childFrame().name());
    }
}


TEST(FrameReferences, DynamicParentStaticChild)
{
    using DynamicFromJupiter = FrameReferences<DynamicFrame, StaticFrame<Jupiter>>;
    EXPECT_NO_THROW(DynamicFromJupiter("DynamicMars"));

    {
        // static_assert(DynamicFromJupiter::ParentFrame::name() == "");
        static_assert(DynamicFromJupiter::ChildFrame::name() == "naksh::geometry::Jupiter");
    }

    {
        const DynamicFromJupiter dynamicFromJupiter("DynamicSaturn");
        EXPECT_EQ("DynamicSaturn", dynamicFromJupiter.parentFrame().name());
        EXPECT_EQ("naksh::geometry::Jupiter", decltype(dynamicFromJupiter)::ChildFrame::name());
    }

    {
        const DynamicFromJupiter dynamicFromJupiter(DynamicFrame("DynamicSaturn"));
        EXPECT_EQ("DynamicSaturn", dynamicFromJupiter.parentFrame().name());
        EXPECT_EQ("naksh::geometry::Jupiter", decltype(dynamicFromJupiter)::ChildFrame::name());
    }
}


TEST(FrameReferences, DynamicParentDynamicChild)
{
    using DynamicFromDynamic = FrameReferences<DynamicFrame, DynamicFrame>;
    EXPECT_NO_THROW(DynamicFromDynamic("DynamicMars", "DynamicNeptune"));

    {
        // static_assert(DynamicFromDynamic::ParentFrame::name() == "");
        // static_assert(DynamicFromDynamic::ChildFrame::name() == "");
    }

    {
        const DynamicFromDynamic dynamicFromDynamic("DynamicMars", "DynamicNeptune");
        EXPECT_EQ("DynamicMars", dynamicFromDynamic.parentFrame().name());
        EXPECT_EQ("DynamicNeptune", dynamicFromDynamic.childFrame().name());
    }

    {
        const DynamicFromDynamic dynamicFromDynamic(DynamicFrame("DynamicMars"),
                                                    DynamicFrame("DynamicNeptune"));

        EXPECT_EQ("DynamicMars", dynamicFromDynamic.parentFrame().name());
        EXPECT_EQ("DynamicNeptune", dynamicFromDynamic.childFrame().name());
    }
}


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


TEST(composeFrameReferences, StaticStaticStaticStatic)
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


TEST(composeFrameReferences, StaticStaticStaticDynamic)
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


TEST(composeFrameReferences, StaticStaticDynamicStatic)
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


TEST(composeFrameReferences, StaticDynamicStaticStatic)
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


TEST(composeFrameReferences, StaticDynamicDynamicStatic)
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


TEST(composeFrameReferences, DynamicStaticStaticDynamic)
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


TEST(invertFrameReferences, allCases)
{
    using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
    using UranusFromDynamicFrames = FrameReferences<StaticFrame<Uranus>, DynamicFrame>;
    using DynamicFromUranusFrames = FrameReferences<DynamicFrame, StaticFrame<Uranus>>;
    using DynamicFromDynamicFrames = FrameReferences<DynamicFrame, DynamicFrame>;

    {
        SunFromUranusFrames input;
        auto result = invertFrameReferences(input);
        using ResultType = decltype(result);

        static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Uranus>>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Sun>>);
    }

    {
        UranusFromDynamicFrames input("naksh::geometry::Pluto");
        auto result = invertFrameReferences(input);
        using ResultType = decltype(result);

        static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Uranus>>);
        EXPECT_EQ(result.parentFrame().name(), "naksh::geometry::Pluto");
    }

    {
        DynamicFromUranusFrames input("naksh::geometry::Sun");
        auto result = invertFrameReferences(input);
        using ResultType = decltype(result);

        static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Uranus>>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
        EXPECT_EQ(result.childFrame().name(), "naksh::geometry::Sun");
    }

    {
        DynamicFromDynamicFrames input("naksh::geometry::Sun", "naksh::geometry::Uranus");
        auto result = invertFrameReferences(input);
        using ResultType = decltype(result);

        static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
        static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
        EXPECT_EQ(result.parentFrame().name(), "naksh::geometry::Uranus");
        EXPECT_EQ(result.childFrame().name(), "naksh::geometry::Sun");
    }
}


} // namespace naksh::geometry

#pragma clang diagnostic pop
