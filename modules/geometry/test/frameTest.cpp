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
  const auto df = DynamicFrame{ "testFrame145" };
  EXPECT_EQ("testFrame145", df.name());
}

TEST(DynamicFrame, EqualityCheck)
{
  const auto d1 = DynamicFrame{ "Test125" };

  {
    const auto d2 = DynamicFrame{ "Test125" };
    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 != d2);
  }

  {
    const auto d3 = DynamicFrame{ "Test43" };
    EXPECT_FALSE(d1 == d3);
    EXPECT_TRUE(d1 != d3);
  }
}


} // namespace nioc::geometry
