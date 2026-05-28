////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/common/utils.hpp>

namespace nioc::common
{

TEST(ProgramName, stripsLeadingDirectories)
{
  const char* const argV[] = { "/usr/local/bin/port" };
  EXPECT_EQ(programName(1, argV), "port");
}

TEST(ProgramName, returnsBareNameUnchanged)
{
  const char* const argV[] = { "port" };
  EXPECT_EQ(programName(1, argV), "port");
}

TEST(ProgramName, ignoresTrailingArguments)
{
  const char* const argV[] = { "/usr/local/bin/port", "--help" };
  EXPECT_EQ(programName(2, argV), "port");
}

TEST(ProgramName, returnsEmptyWhenNoArguments)
{
  EXPECT_EQ(programName(0, nullptr), "");
}

} // namespace nioc::common
