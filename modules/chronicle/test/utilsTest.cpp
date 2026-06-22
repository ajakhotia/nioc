////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <gtest/gtest.h>

namespace nioc::chronicle
{

TEST(ChronicleUtils, padString)
{
  EXPECT_EQ("0000000000682", padString("682", 13U, '0'));
  EXPECT_EQ("682", padString("682", 2U, '0'));
  EXPECT_EQ("ZZZZZZZZZZ682", padString("682", 13U, 'Z'));
}

TEST(ChronicleUtils, buildRollName)
{
  EXPECT_EQ("roll00000000000000000000.nioc", buildRollName(0U));
  EXPECT_EQ("roll00000000000000000001.nioc", buildRollName(1U));
  EXPECT_EQ("roll00000003519894239162.nioc", buildRollName(3519894239162U));
}

TEST(ChronicleUtils, hexString)
{
  EXPECT_EQ("0xff", hexString(255U));
}

} // namespace nioc::chronicle
