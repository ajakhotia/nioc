////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/schemaId.hpp>
#include <string_view>

namespace nioc::terminus
{
namespace
{

chronicle::ChannelId channelFor(const std::string_view topic)
{
  return chronicle::makeChannelId(kSchemaId<TestSchema>, topic);
}

} // namespace

TEST(SchemaId, derivesAStableChannelThatSeparatesTopicsAndSchemas)
{
  const auto channel = channelFor("imu");

  // The same schema and topic always derive the same channel.
  EXPECT_EQ(channel, channelFor("imu"));

  // A different topic, or a different schema id, lands on a different channel.
  EXPECT_NE(channel, channelFor("gps"));
  EXPECT_NE(channel, chronicle::makeChannelId(kSchemaId<TestSchema> + 1, "imu"));
}

} // namespace nioc::terminus
