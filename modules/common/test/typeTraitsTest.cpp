////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <deque>
#include <gtest/gtest.h>
#include <nioc/common/typeTraits.hpp>
#include <vector>

namespace nioc::common
{
TEST(TypeTraits, IsSpecialization)
{
  static_assert(IsSpecialization<std::vector<int>, std::vector>::value);
  static_assert(not(IsSpecialization<std::vector<int>, std::deque>::value));
  static_assert(isSpecialization<std::vector<int>, std::vector>);
  static_assert(not(isSpecialization<std::vector<int>, std::deque>));

  EXPECT_TRUE(bool(IsSpecialization<std::vector<int>, std::vector>::value));
  EXPECT_FALSE(bool(IsSpecialization<std::vector<int>, std::deque>::value));
  EXPECT_TRUE(bool(isSpecialization<std::vector<int>, std::vector>));
  EXPECT_FALSE(bool(isSpecialization<std::vector<int>, std::deque>));
}


class TestType;
class AnotherTestType;

TEST(TypeTraits, prettyName)
{
  static_assert("nioc::common::TestType" == prettyName<TestType>());
  static_assert("nioc::common::TestType" != prettyName<AnotherTestType>());
  EXPECT_EQ("nioc::common::TestType", prettyName<TestType>());
  EXPECT_NE("nioc::common::TestType", prettyName<AnotherTestType>());
}


} // namespace nioc::common
