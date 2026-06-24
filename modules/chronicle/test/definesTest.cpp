////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <unordered_set>

namespace nioc::chronicle
{
namespace
{

TEST(MakeChannelId, isDeterministicAndSeparatesTypesAndTopics)
{
  const auto channel = makeChannelId(42, "imu");

  // Equal (typeId, topic) inputs always map to the same channel.
  EXPECT_EQ(channel, makeChannelId(42, "imu"));

  // A different topic, or a different type id, lands on a different channel.
  EXPECT_NE(channel, makeChannelId(42, "gps"));
  EXPECT_NE(channel, makeChannelId(43, "imu"));
}

TEST(ChannelId, equalsAndHashesByValueSoItKeysAHashSet)
{
  auto seen = std::unordered_set<ChannelId>{};
  seen.insert(makeChannelId(1, "alpha"));
  seen.insert(makeChannelId(1, "alpha")); // same channel: no new entry
  seen.insert(makeChannelId(2, "alpha"));

  EXPECT_EQ(seen.size(), 2U);
  EXPECT_TRUE(seen.contains(makeChannelId(1, "alpha")));
  EXPECT_FALSE(seen.contains(makeChannelId(1, "beta")));
}

} // namespace
} // namespace nioc::chronicle
