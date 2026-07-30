////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <nioc/common/utils.hpp>
#include <stdexcept>

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

TEST(HexString, rendersLowercaseWithA0xPrefix)
{
  EXPECT_EQ("0xff", hexString(255U));
  EXPECT_EQ("0x0", hexString(0U));
}

TEST(FromHexString, invertsHexString)
{
  EXPECT_EQ(255ULL, fromHexString<std::uint64_t>("0xff"));
  EXPECT_EQ(0ULL, fromHexString<std::uint64_t>("0x0"));

  constexpr auto kId = std::uint64_t{0xa32f91dd180cb931ULL};
  EXPECT_EQ(kId, fromHexString<std::uint64_t>(hexString(kId)));

  // The type parsed into is the caller's choice, symmetric with hexString.
  EXPECT_EQ(std::uint8_t{0xff}, fromHexString<std::uint8_t>("0xff"));
}

TEST(FromHexString, rejectsGarbageAndOverflow)
{
  EXPECT_THROW((void)fromHexString<std::uint64_t>("0xnope"), std::invalid_argument);
  EXPECT_THROW((void)fromHexString<std::uint64_t>("0x12zz"), std::invalid_argument);
  EXPECT_THROW((void)fromHexString<std::uint8_t>("0x1ff"), std::out_of_range);
}

} // namespace nioc::common
