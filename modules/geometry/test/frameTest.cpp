////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/geometry/frame.hpp>
#include <type_traits>

namespace nioc::geometry
{
class Mercury;
class Venus;

TEST(StaticFrame, isAPureTypeLevelToken)
{
  // A static frame is never instantiated: its identity is its type, so every special member is
  // deleted and a StaticFrame cannot wrap another StaticFrame.
  using Frame = StaticFrame<Mercury>;
  static_assert(not std::is_default_constructible_v<Frame>);
  static_assert(not std::is_copy_constructible_v<Frame>);
  static_assert(not std::is_move_constructible_v<Frame>);
  static_assert(not std::is_copy_assignable_v<Frame>);
  static_assert(not std::is_move_assignable_v<Frame>);
  static_assert(not std::is_destructible_v<Frame>);
}

TEST(StaticFrame, name)
{
  static_assert("nioc::geometry::Mercury" == StaticFrame<Mercury>::name());
  EXPECT_EQ("nioc::geometry::Venus", StaticFrame<Venus>::name());
}

TEST(DynamicFrame, construction)
{
  EXPECT_NO_THROW(DynamicFrame("testFrame145"));
}

TEST(DynamicFrame, name)
{
  const auto frame = DynamicFrame{"testFrame145"};
  EXPECT_EQ("testFrame145", frame.name());
}

TEST(DynamicFrame, EqualityCheck)
{
  const auto frameOne = DynamicFrame{"Test125"};

  {
    const auto frameTwo = DynamicFrame{"Test125"};
    EXPECT_TRUE(frameOne == frameTwo);
    EXPECT_FALSE(frameOne != frameTwo);
  }

  {
    const auto frameThree = DynamicFrame{"Test43"};
    EXPECT_FALSE(frameOne == frameThree);
    EXPECT_TRUE(frameOne != frameThree);
  }
}


} // namespace nioc::geometry
