////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <nioc/geometry/frameConcepts.hpp>

namespace nioc::geometry
{
class Earth;
class Mars;

TEST(ParentConceptTmpl, StaticConstruction)
{
  EXPECT_NO_THROW(ParentConceptTmpl<StaticFrame<Earth>>());
}

TEST(ParentConceptTmpl, DynamicConstruction)
{
  EXPECT_NO_THROW(ParentConceptTmpl<DynamicFrame>("Test845"));
  EXPECT_NO_THROW(ParentConceptTmpl<DynamicFrame>(DynamicFrame("Test943")));
}

TEST(ParentConceptTmpl, StaticParentFrame)
{
  using ParentEarth = ParentConceptTmpl<StaticFrame<Earth>>;
  static_assert(std::is_same_v<StaticFrame<Earth>, ParentEarth::ParentFrame>);
  EXPECT_EQ(StaticFrame<Earth>::name(), ParentEarth::ParentFrame::name());
}

TEST(ParentConceptTmpl, DynamicParentFrame)
{
  ParentConceptTmpl<DynamicFrame> p1("Test845");
  ParentConceptTmpl<DynamicFrame> p2(DynamicFrame("Test943"));

  EXPECT_EQ(DynamicFrame("Test845"), p1.parentFrame());
  EXPECT_EQ(DynamicFrame("Test943"), p2.parentFrame());
}

TEST(ChildConceptTmpl, StaticConstruction)
{
  EXPECT_NO_THROW(ChildConceptTmpl<StaticFrame<Mars>>());
}

TEST(ChildConceptTmpl, DynamicConstruction)
{
  EXPECT_NO_THROW(ChildConceptTmpl<DynamicFrame>("Test845"));
  EXPECT_NO_THROW(ChildConceptTmpl<DynamicFrame>(DynamicFrame("Test943")));
}

TEST(ParentConceptTmpl, StaticChildFrame)
{
  using ChildMars = ChildConceptTmpl<StaticFrame<Mars>>;
  static_assert(std::is_same_v<StaticFrame<Mars>, ChildMars::ChildFrame>);
  EXPECT_EQ(StaticFrame<Mars>::name(), ChildMars::ChildFrame::name());
}

TEST(ChildConceptTmpl, DynamicChildFrame)
{
  ChildConceptTmpl<DynamicFrame> p1("Test845");
  ChildConceptTmpl<DynamicFrame> p2(DynamicFrame("Test943"));

  EXPECT_EQ(DynamicFrame("Test845"), p1.childFrame());
  EXPECT_EQ(DynamicFrame("Test943"), p2.childFrame());
}


} // End of namespace nioc::geometry.

#pragma clang diagnostic pop
