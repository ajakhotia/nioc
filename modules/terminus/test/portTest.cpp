////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <atomic>
#include <boost/program_options.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/reader.hpp>
#include <nioc/common/filesystem.hpp>
#include <nioc/common/typeTraits.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

fs::path testDataDir()
{
  return fs::path{"data"};
}

fs::path malformedConfig()
{
  return testDataDir() / "malformedConfig.json";
}

fs::path resource()
{
  return testDataDir() / "testResource.bin";
}

fs::path resourceDuplicate()
{
  return testDataDir() / "duplicate" / "testResource.bin";
}

std::string sampleCommandLine()
{
  return "myRobot --config /etc/foo.json";
}

void emptySetup(
    Port& /*unused*/,
    Port::Drivers& /*unused*/,
    Port::Components& /*unused*/,
    Port::Runners& /*unused*/)
{
}

void publishGap(Port& port, const std::string_view topic)
{
  auto publisher = port.publisher<TestSchema>(topic);
  publisher.publish(publisher.draft());
}

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class PortTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  PortTest(): ScratchDirectory{unitTestDirectory()} {}

protected:
  [[nodiscard]] fs::path logRoot() const
  {
    return path() / "logs";
  }

  /// A working directory unique to the running test, not yet created; the test that asserts a
  /// Port creates it depends on that.
  [[nodiscard]] fs::path testWorkingDir() const
  {
    return path() / "work";
  }

  [[nodiscard]] RunContext testRunContext(
      std::string commandLine = "",
      const bool recordChronicle = true,
      std::vector<fs::path> resourcePaths = {},
      std::vector<fs::path> appendConfigPaths = {}) const
  {
    return RunContext{
        testWorkingDir(),
        std::move(resourcePaths),
        recordChronicle,
        std::move(commandLine),
        {},
        std::move(appendConfigPaths)};
  }
};

} // namespace

TEST_F(PortTest, constructionCreatesRecordingDirectory)
{
  const auto workingDir = [&]
  {
    auto port = Port{testRunContext(sampleCommandLine()), emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(recordingDir));
    EXPECT_TRUE(fs::is_directory(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "configOverlay.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "manifest.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "console.log"));
    return recordingDir;
  }();
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST_F(PortTest, recordingCarriesManifestAndResources)
{
  const auto workingDir = [&]
  {
    auto port = Port{testRunContext(sampleCommandLine()), emptySetup};
    const auto recordingDir = port.workingDir();
    port.addResource(resource());

    EXPECT_TRUE(fs::is_regular_file(recordingDir / "testResource.bin"));
    return recordingDir;
  }();

  const auto manifest = nlohmann::json::parse(std::ifstream(workingDir / "manifest.json"));
  EXPECT_EQ(manifest.at("cmdline").get<std::string>(), sampleCommandLine());
  EXPECT_EQ(manifest.at("mode").get<std::string>(), "online");

  // resources.json is rewritten at teardown, so it carries the resource added mid-run.
  const auto resources = nlohmann::json::parse(std::ifstream(workingDir / "resources.json"));
  EXPECT_EQ(resources.at(resource().string()).get<std::string>(), "testResource.bin");
}

TEST_F(PortTest, recordingWritesItsTopicRegistry)
{
  const auto workingDir = [&]
  {
    auto port = Port{
        testRunContext(sampleCommandLine()),
        [](Port& port, Port::Drivers&, Port::Components&, Port::Runners&)
        {
          static_cast<void>(port.publisher<TestSchema>("alpha"));
          static_cast<void>(port.publisher<TestSchema>("beta"));
        }};
    EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "topics.json"));
    return port.workingDir();
  }();

  // The written registry reads back with both topics and their full schema identity. Membership is
  // by whole Topic, so a match confirms every field round-tripped.
  const auto registry = TopicRegistry{workingDir};
  ASSERT_EQ(2U, registry.size());

  const auto alpha = Topic{
      .mChannelId = chronicle::makeChannelId(kSchemaId<TestSchema>, "alpha"),
      .mName = "alpha",
      .mSchemaId = kSchemaId<TestSchema>,
      .mSchemaName = std::string{common::prettyName<TestSchema>()}};
  EXPECT_TRUE(registry.contains(alpha));
}

TEST_F(PortTest, playbackAdoptsTheReplayedRecordingsTopics)
{
  const auto base = testWorkingDir();

  const auto recordingDir = [&]
  {
    auto port = Port{
        RunContext{base / "recording", {}, true, sampleCommandLine()},
        [](Port& port, Port::Drivers&, Port::Components&, Port::Runners&)
        { static_cast<void>(port.publisher<TestSchema>("alpha")); }};
    return port.workingDir();
  }();

  auto port = Port{RunContext{base / "playback", {}, false, "", recordingDir}, emptySetup};

  const auto alpha = Topic{
      .mChannelId = chronicle::makeChannelId(kSchemaId<TestSchema>, "alpha"),
      .mName = "alpha",
      .mSchemaId = kSchemaId<TestSchema>,
      .mSchemaName = std::string{common::prettyName<TestSchema>()}};
  EXPECT_TRUE(port.playbackTopics().contains(alpha));
}

TEST_F(PortTest, aLiveRunReplaysNothingSoItsPlaybackRegistryIsEmpty)
{
  auto port = Port{testRunContext(sampleCommandLine()), emptySetup};
  EXPECT_TRUE(port.playbackTopics().empty());
}

TEST_F(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  auto port = Port{testRunContext(), emptySetup};
  port.addResource(resource());

  const auto acquired = port.acquireResource(resource());
  EXPECT_EQ(acquired, port.workingDir() / "testResource.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST_F(PortTest, acquireResourceRejectsUnaddedResource)
{
  auto port = Port{testRunContext(), emptySetup};
  EXPECT_THROW((void)port.acquireResource(testDataDir() / "never.bin"), std::invalid_argument);
}

TEST_F(PortTest, addResourceRejectsBasenameCollision)
{
  auto port = Port{testRunContext(), emptySetup};
  port.addResource(resource());
  EXPECT_THROW(port.addResource(resourceDuplicate()), std::invalid_argument);
}

TEST_F(PortTest, addResourceRejectsMissingFile)
{
  auto port = Port{testRunContext(), emptySetup};
  EXPECT_THROW(port.addResource(testDataDir() / "doesNotExist"), std::invalid_argument);
}

TEST_F(PortTest, constructionCreatesTheWorkingDirectory)
{
  const auto expectedDir = testWorkingDir();
  ASSERT_FALSE(fs::exists(expectedDir));

  auto port = Port{testRunContext(), emptySetup};

  EXPECT_EQ(port.workingDir(), expectedDir);
  EXPECT_TRUE(fs::is_directory(expectedDir));
}

TEST_F(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  // Without recording there is no chronicle writer; producers build messages on the heap instead.
  const auto workingDir = [&]
  {
    auto port = Port{testRunContext("", false), emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "configOverlay.json"));
    return recordingDir;
  }();
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST_F(PortTest, constructionAddsListedResources)
{
  auto port = Port{testRunContext("", true, {resource()}), emptySetup};
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
}

TEST_F(PortTest, constructionFromCommandLineReadsEveryOption)
{
  // Stage two config layers, then build an argv that exercises every run-context option, mirroring
  // a real command line.
  const auto stagingDir = path() / "cli";
  fs::create_directories(stagingDir);
  const auto base = stagingDir / "base.json";
  std::ofstream(base) << R"({"name": "base", "count": 1})";
  const auto overlay = stagingDir / "overlay.json";
  std::ofstream(overlay) << R"({"count": 2})";

  const auto rootArg = logRoot().string();
  const auto baseArg = base.string();
  const auto overlayArg = overlay.string();
  const auto resourceArg = resource().string();
  const auto argv = std::array<const char*, 13>{
      "myRobot",
      "--log-root",
      rootArg.c_str(),
      "--append-config",
      baseArg.c_str(),
      "--append-config",
      overlayArg.c_str(),
      "--config-override",
      "name=cli",
      "--append-resource",
      resourceArg.c_str(),
      "--record-chronicle",
      "false"};
  constexpr auto argc = static_cast<int>(argv.size());

  const auto variableMap = parseCommandLine(argc, argv.data(), RunContext::cliOptions());

  // parseCommandLine injects the verbatim command line for the Port to record.
  EXPECT_TRUE(variableMap.contains("commandLine"));

  auto port = Port{RunContext{variableMap}, emptySetup};

  EXPECT_EQ(port.workingDir().parent_path(), logRoot()); // minted under the log root
  EXPECT_FALSE(port.runContext().playback());
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false

  // The recorded overrides carry the file merge with the cli override applied on top.
  const auto onDisk = nlohmann::json::parse(
      std::ifstream(port.workingDir() / "configOverlay.json"));
  EXPECT_EQ(onDisk.at("count").get<int>(), 2);            // later file wins
  EXPECT_EQ(onDisk.at("name").get<std::string>(), "cli"); // --config-override wins over files
}

TEST_F(PortTest, constructionRejectsUnreadableConfig)
{
  EXPECT_THROW(
      (Port{testRunContext("", true, {}, {testDataDir() / "doesNotExist.json"}), emptySetup}),
      std::runtime_error);
}

TEST_F(PortTest, constructionRejectsMalformedConfig)
{
  EXPECT_THROW(
      (Port{testRunContext("", true, {}, {malformedConfig()}), emptySetup}),
      nlohmann::json::parse_error);
}

TEST_F(PortTest, publishFansOutToEverySubscriberOnTheChannel)
{
  auto port = Port{testRunContext(), emptySetup};

  const auto channelId = chronicle::makeChannelId(kSchemaId<TestSchema>, "fanOut");
  auto firstCount = 0;
  auto secondCount = 0;
  port.subscribe(channelId, [&firstCount](const Consignment&) { ++firstCount; });
  port.subscribe(channelId, [&secondCount](const Consignment&) { ++secondCount; });

  // A different channel's subscriber must not hear the message.
  auto otherCount = 0;
  port.subscribe(
      chronicle::makeChannelId(kSchemaId<TestSchema>, "otherTopic"),
      [&otherCount](const Consignment&) { ++otherCount; });

  publishGap(port, "fanOut");

  EXPECT_EQ(1, firstCount);
  EXPECT_EQ(1, secondCount);
  EXPECT_EQ(0, otherCount);
}

TEST_F(PortTest, shutdownAndAbortTripTheirTokensIndependently)
{
  const auto port = Port{testRunContext(), emptySetup};

  EXPECT_FALSE(port.shutdownToken().stop_requested());
  EXPECT_FALSE(port.abortToken().stop_requested());

  port.shutdown();
  EXPECT_TRUE(port.shutdownToken().stop_requested());
  EXPECT_FALSE(port.abortToken().stop_requested());

  port.abort();
  EXPECT_TRUE(port.abortToken().stop_requested());
}

TEST_F(PortTest, awaitQuiescenceBlocksUntilDeliveredConsignmentsDie)
{
  auto port = Port{testRunContext(), emptySetup};

  const auto channelId = chronicle::makeChannelId(kSchemaId<TestSchema>, "quiescence");
  auto held = std::vector<Consignment>{};
  port.subscribe(
      channelId,
      [&held](Consignment consignment) { held.push_back(std::move(consignment)); });

  publishGap(port, "quiescence");

  auto quiesced = std::atomic<bool>{false};
  auto waiter = std::thread{[&]
                            {
                              port.awaitQuiescence();
                              quiesced.store(true);
                            }};

  // The subscriber still holds the consignment, so the run is not quiescent.
  constexpr auto kSettleTime = std::chrono::milliseconds{50};
  std::this_thread::sleep_for(kSettleTime);
  EXPECT_FALSE(quiesced.load());

  held.clear();
  waiter.join();
  EXPECT_TRUE(quiesced.load());
}

TEST_F(PortTest, abortUnblocksAwaitQuiescenceWithConsignmentsStillHeld)
{
  auto port = Port{testRunContext(), emptySetup};

  const auto channelId = chronicle::makeChannelId(kSchemaId<TestSchema>, "abortQuiescence");
  auto held = std::vector<Consignment>{};
  port.subscribe(
      channelId,
      [&held](Consignment consignment) { held.push_back(std::move(consignment)); });

  publishGap(port, "abortQuiescence");

  auto quiesced = std::atomic<bool>{false};
  auto waiter = std::thread{[&]
                            {
                              port.awaitQuiescence();
                              quiesced.store(true);
                            }};

  constexpr auto kSettleTime = std::chrono::milliseconds{50};
  std::this_thread::sleep_for(kSettleTime);
  EXPECT_FALSE(quiesced.load());

  // The consignment is still alive; abort() alone must release the waiter.
  port.abort();
  waiter.join();
  EXPECT_TRUE(quiesced.load());

  held.clear();
}

TEST_F(PortTest, everyPublishedMessageIsRecordedInOrder)
{
  constexpr auto kMessageCount = std::int64_t{64};
  constexpr auto kTopic = std::string_view{"chronicleGate"};

  // No subscribers: publishing records each message into the chronicle synchronously. Read the
  // recording back and expect every published value, in the publish-order.
  const auto workingDir = [&]
  {
    auto port = Port{testRunContext(), emptySetup};
    auto publisher = port.publisher<TestSchema>(kTopic);
    for(auto value = std::int64_t{0}; value < kMessageCount; ++value)
    {
      auto draft = publisher.draft();
      draft.builder().setValue(value);
      publisher.publish(std::move(draft));
    }
    return port.workingDir();
  }();

  auto reader = chronicle::Reader{workingDir / "chronicle"};
  const auto channelId = chronicle::makeChannelId(kSchemaId<TestSchema>, kTopic);

  auto nextValue = std::int64_t{0};
  for(const auto& entry: reader)
  {
    if(entry.mChannelId != channelId)
    {
      continue;
    }
    const auto loaded = Message<TestSchema>{entry.mCrate};
    EXPECT_EQ(nextValue, loaded.reader().getValue());
    ++nextValue;
  }

  EXPECT_EQ(kMessageCount, nextValue);
}

TEST_F(PortTest, waitReturnsFalseOnceEveryDriverIsDone)
{
  class ScriptedDriver final: public Driver
  {
  public:
    ScriptedDriver(Port& port, const int steps): Driver{"ScriptedDriver", port}, mRemaining{steps}
    {
    }

  private:
    int mRemaining;

    State run() final
    {
      return --mRemaining > 0 ? State::Continue : State::Done;
    }
  };

  auto port = Port{
      testRunContext(),
      [](Port& port, Port::Drivers& drivers, Port::Components&, Port::Runners& runners)
      {
        constexpr auto kSteps = 5;
        auto driver = std::make_shared<ScriptedDriver>(port, kSteps);
        auto runner = std::make_shared<concurrent::ThreadedRunner>();
        runner->launch(driver);
        drivers.push_back(std::move(driver));
        runners.push_back(std::move(runner));
      }};

  // wait() paces the main loop while the driver works and returns false once it reports Done; a
  // missed transition would spin past the deadline.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
  while(port.wait(std::chrono::milliseconds{1}, [] {}))
  {
    ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "wait() never reported done";
  }
}

} // namespace nioc::terminus
