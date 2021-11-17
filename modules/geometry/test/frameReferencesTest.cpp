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
class Jupiter;

namespace helpers
{

TEST(ParentConcept, StaticConstruction)
{
    EXPECT_NO_THROW(ParentConcept<Earth>());
}


TEST(ParentConcept, DynamicConstruction)
{
    EXPECT_NO_THROW(ParentConcept<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ParentConcept<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ParentConcept, DynamicName)
{
    ParentConcept<DynamicFrame> p1("Test845");
    ParentConcept<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.parentFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.parentFrame());
}


TEST(ChildConcept, StaticConstruction)
{
    EXPECT_NO_THROW(ChildConcept<Earth>());
}


TEST(ChildConcept, DynamicConstruction)
{
    EXPECT_NO_THROW(ChildConcept<DynamicFrame>("Test845"));
    EXPECT_NO_THROW(ChildConcept<DynamicFrame>(DynamicFrame("Test943")));
}


TEST(ChildConcept, DynamicName)
{
    ChildConcept<DynamicFrame> p1("Test845");
    ChildConcept<DynamicFrame> p2(DynamicFrame("Test943"));

    EXPECT_EQ(DynamicFrame("Test845"), p1.childFrame());
    EXPECT_EQ(DynamicFrame("Test943"), p2.childFrame());
}

} // End of namespace helpers.


TEST(FrameReferences, StaticParentStaticChild)
{
    using EarthFromJupiter = FrameReferences<Earth, Jupiter>;
    EXPECT_NO_THROW(EarthFromJupiter());

    static_assert(EarthFromJupiter::ParentFrame::name() == "naksh::geometry::Earth");
    static_assert(EarthFromJupiter::ChildFrame::name() == "naksh::geometry::Jupiter");

    const EarthFromJupiter earthFromJupiter;
    EXPECT_EQ("naksh::geometry::Earth", decltype(earthFromJupiter)::ParentFrame::name());
    EXPECT_EQ("naksh::geometry::Jupiter", decltype(earthFromJupiter)::ChildFrame::name());
}


TEST(FrameReferences, StaticParentDynamicChild)
{
    using EarthFromDynamic = FrameReferences<Earth, DynamicFrame>;
    EXPECT_NO_THROW(EarthFromDynamic("DynamicSaturn"));

    {
        static_assert(EarthFromDynamic::ParentFrame::name() == "naksh::geometry::Earth");
        //static_assert(EarthFromDynamic::ChildFrame::name() == "");
    }

    {
        const EarthFromDynamic earthFromDynamic("DynamicSaturn");
        EXPECT_EQ("naksh::geometry::Earth", decltype(earthFromDynamic)::ParentFrame::name());
        EXPECT_EQ("DynamicSaturn", earthFromDynamic.childFrame().name());
    }

    {
        const EarthFromDynamic earthFromDynamic(DynamicFrame("DynamicSaturn"));
        EXPECT_EQ("naksh::geometry::Earth", decltype(earthFromDynamic)::ParentFrame::name());
        EXPECT_EQ("DynamicSaturn", earthFromDynamic.childFrame().name());
    }
}


TEST(FrameReferences, DynamicParentStaticChild)
{
    using DynamicFromJupiter = FrameReferences<DynamicFrame, Jupiter>;
    EXPECT_NO_THROW(DynamicFromJupiter("DynamicMars"));

    {
        //static_assert(DynamicFromJupiter::ParentFrame::name() == "");
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
        //static_assert(DynamicFromDynamic::ParentFrame::name() == "");
        //static_assert(DynamicFromDynamic::ChildFrame::name() == "");
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


} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
