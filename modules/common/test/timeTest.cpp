////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/common/time.hpp>

namespace nioc::common
{

TEST(Iso8601UtcFormat, formatsKnownInstantAsExpectedString)
{
  using std::chrono::system_clock;
  constexpr auto timePoint = system_clock::time_point(system_clock::duration(1756736313992295120));
  EXPECT_EQ("2025-09-01T14:18:33.992295120Z", iso8601UtcFormat(timePoint));
}

} // namespace nioc::common
