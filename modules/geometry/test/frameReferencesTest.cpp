////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/geometry/frameReferences.hpp>

namespace nioc::geometry
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

  static_assert(SaturnFromJupiter::ParentFrame::name() == "nioc::geometry::Saturn");
  static_assert(SaturnFromJupiter::ChildFrame::name() == "nioc::geometry::Jupiter");

  const auto saturnFromJupiter = SaturnFromJupiter{};
  EXPECT_EQ("nioc::geometry::Saturn", decltype(saturnFromJupiter)::ParentFrame::name());
  EXPECT_EQ("nioc::geometry::Jupiter", decltype(saturnFromJupiter)::ChildFrame::name());
}

TEST(FrameReferences, StaticParentDynamicChild)
{
  using SaturnFromDynamic = FrameReferences<StaticFrame<Saturn>, DynamicFrame>;
  EXPECT_NO_THROW(SaturnFromDynamic("DynamicSaturn"));

  {
    static_assert(SaturnFromDynamic::ParentFrame::name() == "nioc::geometry::Saturn");
    // static_assert(SaturnFromDynamic::ChildFrame::name() == "");
  }

  {
    const auto saturnFromDynamic = SaturnFromDynamic{ "DynamicSaturn" };
    EXPECT_EQ("nioc::geometry::Saturn", decltype(saturnFromDynamic)::ParentFrame::name());
    EXPECT_EQ("DynamicSaturn", saturnFromDynamic.childFrame().name());
  }

  {
    const auto saturnFromDynamic = SaturnFromDynamic{ DynamicFrame("DynamicSaturn") };
    EXPECT_EQ("nioc::geometry::Saturn", decltype(saturnFromDynamic)::ParentFrame::name());
    EXPECT_EQ("DynamicSaturn", saturnFromDynamic.childFrame().name());
  }
}

TEST(FrameReferences, DynamicParentStaticChild)
{
  using DynamicFromJupiter = FrameReferences<DynamicFrame, StaticFrame<Jupiter>>;
  EXPECT_NO_THROW(DynamicFromJupiter("DynamicMars"));

  {
    // static_assert(DynamicFromJupiter::ParentFrame::name() == "");
    static_assert(DynamicFromJupiter::ChildFrame::name() == "nioc::geometry::Jupiter");
  }

  {
    const auto dynamicFromJupiter = DynamicFromJupiter{ "DynamicSaturn" };
    EXPECT_EQ("DynamicSaturn", dynamicFromJupiter.parentFrame().name());
    EXPECT_EQ("nioc::geometry::Jupiter", decltype(dynamicFromJupiter)::ChildFrame::name());
  }

  {
    const auto dynamicFromJupiter = DynamicFromJupiter{ DynamicFrame("DynamicSaturn") };
    EXPECT_EQ("DynamicSaturn", dynamicFromJupiter.parentFrame().name());
    EXPECT_EQ("nioc::geometry::Jupiter", decltype(dynamicFromJupiter)::ChildFrame::name());
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
    const auto dynamicFromDynamic = DynamicFromDynamic{ "DynamicMars", "DynamicNeptune" };
    EXPECT_EQ("DynamicMars", dynamicFromDynamic.parentFrame().name());
    EXPECT_EQ("DynamicNeptune", dynamicFromDynamic.childFrame().name());
  }

  {
    const auto dynamicFromDynamic = DynamicFromDynamic{ DynamicFrame("DynamicMars"),
                                                        DynamicFrame("DynamicNeptune") };

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
      (assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("nioc::geometry::Sun"))));

  EXPECT_THROW(
      (assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame("Bloop"))),
      FrameCompositionException);

  static_assert(not noexcept(assertFrameEqual<StaticFrame<Sun>, DynamicFrame>(DynamicFrame(""))));
}

TEST(assertFrameEqual, DynamicLhsStaticRhs)
{
  EXPECT_NO_THROW(
      (assertFrameEqual<DynamicFrame, StaticFrame<Sun>>(DynamicFrame("nioc::geometry::Sun"))));

  EXPECT_THROW(
      (assertFrameEqual<DynamicFrame, StaticFrame<AlphaCentauri>>(DynamicFrame("Bloop"))),
      FrameCompositionException);

  static_assert(not noexcept(assertFrameEqual<DynamicFrame, StaticFrame<Sun>>(DynamicFrame(""))));
}

TEST(assertFrameEqual, DynamicLhsDynamicRhs)
{
  EXPECT_NO_THROW((assertFrameEqual<DynamicFrame, DynamicFrame>(
      DynamicFrame("nioc::geometry::Sun"),
      DynamicFrame("nioc::geometry::Sun"))));

  EXPECT_THROW(
      (assertFrameEqual<DynamicFrame, DynamicFrame>(DynamicFrame("Bloop"), DynamicFrame("Bleep"))),
      FrameCompositionException);

  static_assert(not noexcept(
      assertFrameEqual<DynamicFrame, DynamicFrame>(DynamicFrame(""), DynamicFrame(""))));
}

} // namespace helpers

TEST(composeFrameReferences, StaticStaticStaticStatic)
{
  using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
  using UranusFromPlutoFrames = FrameReferences<StaticFrame<Uranus>, StaticFrame<Pluto>>;

  const auto lhs = SunFromUranusFrames{};
  const auto rhs = UranusFromPlutoFrames{};

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

  const auto lhs = SunFromUranusFrames{};
  const auto rhs = UranusFromDynaimcFrames{ "milkyWay" };

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
    const auto lhs = SunFromUranusFrames{};
    const auto rhs = DynamicFromPlutoFrames{ "nioc::geometry::Uranus" };

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
  }

  {
    const auto lhs = SunFromUranusFrames{};
    const auto rhs = DynamicFromPlutoFrames{ "milkyWay" };

    EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
  }
}

TEST(composeFrameReferences, StaticDynamicStaticStatic)
{
  using SunFromDynamicFrames = FrameReferences<StaticFrame<Sun>, DynamicFrame>;
  using DynamicFromPlutoFrames = FrameReferences<StaticFrame<Uranus>, StaticFrame<Pluto>>;

  {
    const auto lhs = SunFromDynamicFrames{ "nioc::geometry::Uranus" };
    const auto rhs = DynamicFromPlutoFrames{};

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
  }

  {
    const auto lhs = SunFromDynamicFrames{ "milkyWay" };
    const auto rhs = DynamicFromPlutoFrames{};

    EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
  }
}

TEST(composeFrameReferences, StaticDynamicDynamicStatic)
{
  using SunFromDynamicFrames = FrameReferences<StaticFrame<Sun>, DynamicFrame>;
  using DynamicFromPlutoFrames = FrameReferences<DynamicFrame, StaticFrame<Pluto>>;

  {
    const auto lhs = SunFromDynamicFrames{ "nioc::geometry::Uranus" };
    const auto rhs = DynamicFromPlutoFrames{ "nioc::geometry::Uranus" };

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Sun>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Pluto>>);
  }

  {
    const auto lhs = SunFromDynamicFrames{ "MilkyWay" };
    const auto rhs = DynamicFromPlutoFrames{ "Andromeda" };

    EXPECT_THROW(composeFrameReferences(lhs, rhs), FrameCompositionException);
  }
}

TEST(composeFrameReferences, DynamicStaticStaticDynamic)
{
  using DynamicFromUranusFrames = FrameReferences<DynamicFrame, StaticFrame<Uranus>>;
  using UranusFromDynamicFrames = FrameReferences<StaticFrame<Uranus>, DynamicFrame>;

  {
    const auto lhs = DynamicFromUranusFrames{ "nioc::geometry::Sun" };
    const auto rhs = UranusFromDynamicFrames{ "nioc::geometry::Pluto" };

    EXPECT_NO_THROW(composeFrameReferences(lhs, rhs));
    const auto result = composeFrameReferences(lhs, rhs);

    using ResultType = decltype(result);
    static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
    EXPECT_EQ(result.parentFrame().name(), "nioc::geometry::Sun");
    EXPECT_EQ(result.childFrame().name(), "nioc::geometry::Pluto");
  }
}

TEST(invertFrameReferences, allCases)
{
  using SunFromUranusFrames = FrameReferences<StaticFrame<Sun>, StaticFrame<Uranus>>;
  using UranusFromDynamicFrames = FrameReferences<StaticFrame<Uranus>, DynamicFrame>;
  using DynamicFromUranusFrames = FrameReferences<DynamicFrame, StaticFrame<Uranus>>;
  using DynamicFromDynamicFrames = FrameReferences<DynamicFrame, DynamicFrame>;

  {
    const auto input = SunFromUranusFrames{};
    auto result = invertFrameReferences(input);
    using ResultType = decltype(result);

    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Uranus>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Sun>>);
  }

  {
    const auto input = UranusFromDynamicFrames{ "nioc::geometry::Pluto" };
    auto result = invertFrameReferences(input);
    using ResultType = decltype(result);

    static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, StaticFrame<Uranus>>);
    EXPECT_EQ(result.parentFrame().name(), "nioc::geometry::Pluto");
  }

  {
    const auto input = DynamicFromUranusFrames{ "nioc::geometry::Sun" };
    auto result = invertFrameReferences(input);
    using ResultType = decltype(result);

    static_assert(std::is_same_v<typename ResultType::ParentFrame, StaticFrame<Uranus>>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
    EXPECT_EQ(result.childFrame().name(), "nioc::geometry::Sun");
  }

  {
    const auto input = DynamicFromDynamicFrames{ "nioc::geometry::Sun", "nioc::geometry::Uranus" };
    auto result = invertFrameReferences(input);
    using ResultType = decltype(result);

    static_assert(std::is_same_v<typename ResultType::ParentFrame, DynamicFrame>);
    static_assert(std::is_same_v<typename ResultType::ChildFrame, DynamicFrame>);
    EXPECT_EQ(result.parentFrame().name(), "nioc::geometry::Uranus");
    EXPECT_EQ(result.childFrame().name(), "nioc::geometry::Sun");
  }
}


} // namespace nioc::geometry
