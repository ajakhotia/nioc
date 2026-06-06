////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <gtest/gtest.h>
#include <nioc/common/utils.hpp>

namespace nioc::common
{

TEST(ProgramName, stripsLeadingDirectories)
{
  const auto argV = std::array<const char*, 1>{"/usr/local/bin/port"};
  EXPECT_EQ(programName(1, argV.data()), "port");
}

TEST(ProgramName, returnsBareNameUnchanged)
{
  const auto argV = std::array<const char*, 1>{"port"};
  EXPECT_EQ(programName(1, argV.data()), "port");
}

TEST(ProgramName, ignoresTrailingArguments)
{
  const auto argV = std::array<const char*, 2>{"/usr/local/bin/port", "--help"};
  EXPECT_EQ(programName(2, argV.data()), "port");
}

TEST(ProgramName, returnsEmptyWhenNoArguments)
{
  EXPECT_EQ(programName(0, nullptr), "");
}

} // namespace nioc::common
