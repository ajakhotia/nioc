////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/geometry/frame.hpp>

namespace nioc::geometry
{
class Mercury;
class Venus;

TEST(StaticFrame, construction)
{
  // A static frame cannot be constructed as one should never need to.
  // EXPECT_NO_THROW(StaticFrame<Mercury>());
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
