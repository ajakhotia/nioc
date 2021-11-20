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

class Saturn;
class Jupiter;


TEST(FrameReferences, StaticParentStaticChild)
{
    using SaturnFromJupiter = FrameReferences<Saturn, Jupiter>;
    EXPECT_NO_THROW(SaturnFromJupiter());

    static_assert(SaturnFromJupiter::ParentFrame::name() == "naksh::geometry::Saturn");
    static_assert(SaturnFromJupiter::ChildFrame::name() == "naksh::geometry::Jupiter");

    const SaturnFromJupiter saturnFromJupiter;
    EXPECT_EQ("naksh::geometry::Saturn", decltype(saturnFromJupiter)::ParentFrame::name());
    EXPECT_EQ("naksh::geometry::Jupiter", decltype(saturnFromJupiter)::ChildFrame::name());
}


TEST(FrameReferences, StaticParentDynamicChild)
{
    using SaturnFromDynamic = FrameReferences<Saturn, DynamicFrame>;
    EXPECT_NO_THROW(SaturnFromDynamic("DynamicSaturn"));

    {
        static_assert(SaturnFromDynamic::ParentFrame::name() == "naksh::geometry::Saturn");
        //static_assert(SaturnFromDynamic::ChildFrame::name() == "");
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
        const DynamicFromDynamic dynamicFromDynamic(
            DynamicFrame("DynamicMars"),
            DynamicFrame("DynamicNeptune"));

        EXPECT_EQ("DynamicMars", dynamicFromDynamic.parentFrame().name());
        EXPECT_EQ("DynamicNeptune", dynamicFromDynamic.childFrame().name());
    }
}


} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
