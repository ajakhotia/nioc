////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/schema.h>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/chronicle/defines.hpp>
#include <nioc/concurrent/routine.hpp>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/consignment.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/logPlayer.hpp>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

using State = concurrent::Routine::State;

chronicle::ChannelId channelFor(const std::string_view topic)
{
  return chronicle::makeChannelId(kSchemaId<TestSchema>, topic);
}

Port makePort(const std::string_view name, const bool record)
{
  return Port{
      Manifest{
               RunContext{fs::temp_directory_path() / "nioc-logPlayerTest" / name, {}, record, ""},
               ConfigStore{"{}", capnp::Schema::from<TestConfig>()}},
      [](Port&, Port::Drivers&, Port::Components&, Port::Runners&) {}
  };
}

void publishValue(Publisher<TestSchema>& publisher, const std::int64_t value)
{
  auto draft = publisher.draft();
  draft.builder().setValue(value);
  publisher.publish(std::move(draft));
}

void replay(Port& port, const fs::path& chronicleDir)
{
  auto player = LogPlayer{port, chronicleDir};
  while(player.tick() == State::Continue)
  {
  }
}

} // namespace

TEST(LogPlayer, replaysFramesAcrossChannelsInGlobalRecordOrder)
{
  const auto channelA = channelFor("alpha");
  const auto channelB = channelFor("beta");

  // Record two channels interleaved into one chronicle, then close the recording cleanly.
  const auto chronicleDir = [&]
  {
    auto port = makePort("recordOrder", true);
    auto publisherA = port.publisher<TestSchema>("alpha");
    auto publisherB = port.publisher<TestSchema>("beta");

    publishValue(publisherA, 10);
    publishValue(publisherB, 20);
    publishValue(publisherA, 11);
    publishValue(publisherB, 21);
    publishValue(publisherA, 12);

    return port.workingDir() / "chronicle";
  }();

  // Replay onto a fresh online Port and capture the global delivery order across both channels.
  auto port = makePort("replayOrder", false);
  auto received = std::vector<std::pair<chronicle::ChannelId, std::int64_t>>{};
  const auto recorder = [&received](const chronicle::ChannelId channelId)
  {
    return [&received, channelId](Consignment consignment)
    {
      received.emplace_back(
          channelId,
          Message<TestSchema>{consignment.crate()}.reader().getValue());
    };
  };
  port.subscribe(channelA, recorder(channelA));
  port.subscribe(channelB, recorder(channelB));

  replay(port, chronicleDir);

  const auto expected = std::vector<std::pair<chronicle::ChannelId, std::int64_t>>{
      {channelA, 10},
      {channelB, 20},
      {channelA, 11},
      {channelB, 21},
      {channelA, 12}
  };
  EXPECT_EQ(expected, received);
}

TEST(LogPlayer, preservesPayloadAndSequenceNumberAcrossReplay)
{
  const auto channel = channelFor("telemetry");

  const auto chronicleDir = [&]
  {
    auto port = makePort("fidelity", true);
    auto publisher = port.publisher<TestSchema>("telemetry");
    publishValue(publisher, 100);
    publishValue(publisher, 200);
    return port.workingDir() / "chronicle";
  }();

  auto port = makePort("fidelityReplay", false);
  auto values = std::vector<std::int64_t>{};
  auto sequenceNumbers = std::vector<std::uint64_t>{};
  port.subscribe(
      channel,
      [&](Consignment consignment)
      {
        const auto message = Message<TestSchema>{consignment.crate()};
        values.push_back(message.reader().getValue());
        sequenceNumbers.push_back(message.sequenceNumber());
      });

  replay(port, chronicleDir);

  EXPECT_EQ((std::vector<std::int64_t>{100, 200}), values);
  EXPECT_EQ((std::vector<std::uint64_t>{1, 2}), sequenceNumbers);
}

TEST(LogPlayer, replaysAGapAsAGap)
{
  const auto channel = channelFor("gappy");

  const auto chronicleDir = [&]
  {
    auto port = makePort("gap", true);
    auto publisher = port.publisher<TestSchema>("gappy");

    // A draft published without ever calling builder() records a null payload - a gap.
    publisher.publish(publisher.draft());

    return port.workingDir() / "chronicle";
  }();

  auto port = makePort("gapReplay", false);
  auto gaps = std::vector<bool>{};
  port.subscribe(
      channel,
      [&](Consignment consignment)
      { gaps.push_back(Message<TestSchema>{consignment.crate()}.isGap()); });

  replay(port, chronicleDir);

  EXPECT_EQ((std::vector<bool>{true}), gaps);
}

TEST(LogPlayer, anEmptyLogDeliversNothingAndFinishesImmediately)
{
  const auto chronicleDir = [&]
  {
    const auto port = makePort("empty", true);
    return port.workingDir() / "chronicle";
  }();

  auto port = makePort("emptyReplay", false);
  auto deliveries = 0;
  port.subscribe(channelFor("unused"), [&deliveries](Consignment) { ++deliveries; });

  auto player = LogPlayer{port, chronicleDir};
  EXPECT_EQ(player.tick(), State::Done);
  EXPECT_EQ(deliveries, 0);
}

TEST(LogPlayer, constructionRejectsMissingLog)
{
  auto port = makePort("missing", false);
  const auto absent = fs::temp_directory_path() / "nioc-logPlayerTest" / "absent-chronicle";
  fs::remove_all(absent);
  EXPECT_THROW((LogPlayer{port, absent}), std::invalid_argument);
}

} // namespace nioc::terminus
