////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// A fresh, unique working directory for the running test, cleared of any prior run.
fs::path testDir()
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  const auto dir = fs::temp_directory_path() / "niocTopicRegistryTest" / info->name();
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

chronicle::ChannelId channel(const std::uint64_t value)
{
  return chronicle::ChannelId{value};
}

/// The topic recorded on @p channelId, or nullptr if none. The set's `find` compares every field,
/// so a test inspecting one topic by channel alone scans for it.
const Topic* find(const TopicRegistry& registry, const chronicle::ChannelId channelId)
{
  for(const auto& topic: registry)
  {
    if(topic.mChannelId == channelId)
    {
      return &topic;
    }
  }
  return nullptr;
}

// Stand-in channel and schema ids; the registry never interprets them, so any distinct values do.
constexpr auto kImuChannel = std::uint64_t{0x11ULL};
constexpr auto kGnssChannel = std::uint64_t{0x22ULL};
constexpr auto kImuSchemaId = std::uint64_t{0xa32f91dd180cb931ULL};
constexpr auto kGnssSchemaId = std::uint64_t{0x0d1eULL};

} // namespace

TEST(TopicRegistry, aDefaultRegistryIsEmpty)
{
  const auto registry = TopicRegistry{};
  EXPECT_TRUE(registry.empty());
}

TEST(TopicRegistry, aRecordedTopicRoundTripsThroughDisk)
{
  const auto dir = testDir();

  auto written = TopicRegistry{};
  written.record(channel(kImuChannel), "/imu", kImuSchemaId, "nioc::sensors::Imu");
  written.record(channel(kGnssChannel), "/gnss", kGnssSchemaId, "nioc::sensors::Gnss");
  written.write(dir);

  const auto read = TopicRegistry{dir};
  ASSERT_EQ(2U, read.size());

  const auto* const imu = find(read, channel(kImuChannel));
  ASSERT_NE(nullptr, imu);
  EXPECT_EQ(channel(kImuChannel), imu->mChannelId);
  EXPECT_EQ("/imu", imu->mName);
  EXPECT_EQ(kImuSchemaId, imu->mSchemaId);
  EXPECT_EQ("nioc::sensors::Imu", imu->mSchemaName);
}

TEST(TopicRegistry, recordingAChannelTwiceThrowsEvenForTheIdenticalTopic)
{
  auto registry = TopicRegistry{};
  registry.record(channel(kImuChannel), "/imu", kImuSchemaId, "nioc::sensors::Imu");

  // A channel takes one publisher, so a repeat is a second publisher on it, which is illegal even
  // when the topic is byte-for-byte the same.
  EXPECT_THROW(
      registry.record(channel(kImuChannel), "/imu", kImuSchemaId, "nioc::sensors::Imu"),
      std::runtime_error);
  EXPECT_EQ(1U, registry.size());
}

TEST(TopicRegistry, reusingAChannelForADifferentTopicThrows)
{
  auto registry = TopicRegistry{};
  registry.record(channel(kImuChannel), "/imu", kImuSchemaId, "nioc::sensors::Imu");

  // Same channel, different metadata: a channel is meant to identify one topic, so this is a clash.
  EXPECT_THROW(
      registry.record(channel(kImuChannel), "/imu", kGnssSchemaId, "nioc::sensors::Gnss"),
      std::runtime_error);
}

TEST(TopicRegistry, anEmptyPathAdoptsNothing)
{
  const auto registry = TopicRegistry{fs::path{}};
  EXPECT_TRUE(registry.empty());
}

TEST(TopicRegistry, aDirectoryWithoutARecordIsAdoptedAsEmpty)
{
  const auto registry = TopicRegistry{testDir()};
  EXPECT_TRUE(registry.empty());
}

TEST(TopicRegistry, aMalformedRecordIsAdoptedAsEmpty)
{
  const auto dir = testDir();
  std::ofstream{dir / "topics.json"} << "{ this is not json";

  const auto registry = TopicRegistry{dir};
  EXPECT_TRUE(registry.empty());
}

TEST(TopicRegistry, streamingEmitsAHeadingRowAndOneRowPerTopic)
{
  auto registry = TopicRegistry{};
  registry.record(channel(kImuChannel), "/imu", kImuSchemaId, "nioc::sensors::Imu");
  registry.record(channel(kGnssChannel), "/gnss", kGnssSchemaId, "nioc::sensors::Gnss");

  auto stream = std::ostringstream{};
  stream << registry;
  const auto table = stream.str();

  // A header naming the columns, then the two topics, and the ids rendered as hex.
  EXPECT_NE(std::string::npos, table.find("topic"));
  EXPECT_NE(std::string::npos, table.find("schemaId"));
  EXPECT_NE(std::string::npos, table.find("/imu"));
  EXPECT_NE(std::string::npos, table.find("/gnss"));
  EXPECT_NE(std::string::npos, table.find("0xa32f91dd180cb931"));
  EXPECT_EQ(3, std::ranges::count(table, '\n')); // header plus two rows
}

} // namespace nioc::terminus
