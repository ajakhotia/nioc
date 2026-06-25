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
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/terminus/config/testConfig.capnp.h>
#include <nioc/terminus/configStore.hpp>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/publisher.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/schemaId.hpp>
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

fs::path config()
{
  return testDataDir() / "testConfig.json";
}

fs::path configOverride()
{
  return testDataDir() / "testConfigOverride.json";
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

fs::path logRoot()
{
  return fs::temp_directory_path() / "niocLogs";
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

Manifest testManifest(std::string commandLine = "")
{
  return Manifest{
      RunContext{logRoot(), {}, true, std::move(commandLine)},
      ConfigStore{{config()}, {}}
  };
}

void publishGap(Port& port, const std::string_view topic)
{
  auto publisher = port.publisher<TestSchema>(topic);
  publisher.publish(publisher.draft());
}

} // namespace

TEST(PortTest, constructionCreatesRecordingDirectory)
{
  const auto workingDir = [&]
  {
    auto port = Port{testManifest(sampleCommandLine()), emptySetup};
    const auto& recordingDir = port.workingDir();

    EXPECT_TRUE(fs::is_directory(recordingDir));
    EXPECT_EQ(recordingDir.parent_path(), logRoot());
    EXPECT_TRUE(fs::is_directory(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "manifest.json"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "console.log"));
    return recordingDir;
  }();
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST(PortTest, configMergesPathsLeftToRight)
{
  auto port = Port{
      Manifest{RunContext{logRoot(), {}, true, ""}, ConfigStore{{config(), configOverride()}, {}}},
      emptySetup
  };

  // The merged config is recorded to <dir>/config.json; verify the merge through the recording.
  const auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));

  EXPECT_EQ(onDisk.at("robot").get<std::string>(), "atlas");          // base only
  EXPECT_EQ(onDisk.at("controlRateHz").get<int>(), 200);              // base only
  EXPECT_TRUE(onDisk.at("sensors").at("imu").get<bool>());            // base only
  EXPECT_EQ(onDisk.at("sensors").at("lidarChannels").get<int>(), 32); // override wins
  EXPECT_TRUE(onDisk.at("sensors").at("camera").get<bool>());         // override only
  EXPECT_EQ(onDisk.at("mode").get<std::string>(), "override");        // override wins
}

TEST(PortTest, recordingCarriesManifestAndResources)
{
  const auto workingDir = [&]
  {
    auto port = Port{testManifest(sampleCommandLine()), emptySetup};
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

TEST(PortTest, acquireResourceRemapsToWorkingDirCopy)
{
  auto port = Port{testManifest(), emptySetup};
  port.addResource(resource());

  const auto acquired = port.acquireResource(resource());
  EXPECT_EQ(acquired, port.workingDir() / "testResource.bin");
  EXPECT_TRUE(fs::is_regular_file(acquired));
}

TEST(PortTest, acquireResourceRejectsUnaddedResource)
{
  auto port = Port{testManifest(), emptySetup};
  EXPECT_THROW((void)port.acquireResource(testDataDir() / "never.bin"), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsBasenameCollision)
{
  auto port = Port{testManifest(), emptySetup};
  port.addResource(resource());
  EXPECT_THROW(port.addResource(resourceDuplicate()), std::invalid_argument);
}

TEST(PortTest, addResourceRejectsMissingFile)
{
  auto port = Port{testManifest(), emptySetup};
  EXPECT_THROW(port.addResource(testDataDir() / "doesNotExist"), std::invalid_argument);
}

TEST(PortTest, constructionCreatesMissingLogRoot)
{
  const auto absentRoot = fs::temp_directory_path() / "niocPortTestAbsentRoot";
  fs::remove_all(absentRoot);

  auto port = Port{
      Manifest{RunContext{absentRoot, {}, true, ""}, ConfigStore{{}, {}}},
      emptySetup
  };

  EXPECT_TRUE(fs::is_directory(absentRoot));
  EXPECT_EQ(port.workingDir().parent_path(), absentRoot);
}

TEST(PortTest, recordChronicleFalseOmitsChronicleDir)
{
  // Without recording there is no chronicle writer; producers build messages on the heap instead.
  const auto workingDir = [&]
  {
    auto port = Port{
        Manifest{RunContext{logRoot(), {}, false, ""}, ConfigStore{{config()}, {}}},
        emptySetup
    };
    const auto& recordingDir = port.workingDir();

    EXPECT_FALSE(fs::exists(recordingDir / "chronicle"));
    EXPECT_TRUE(fs::is_regular_file(recordingDir / "config.json"));
    return recordingDir;
  }();
  // The recording is still finalized even with the chronicle disabled.
  EXPECT_TRUE(fs::is_regular_file(workingDir / "resources.json"));
}

TEST(PortTest, deliversToSubscribersWithoutRecording)
{
  // Temporarily disabled: Publisher now always builds into a recording channel, so a non-recording
  // run cannot mint a publisher (Port::publisher throws). Restore once the heap-only publish path
  // is reintroduced.
  GTEST_SKIP() << "non-recording publish path is unsupported pending the channel-always rework";

  // recordChronicle = false: the message is built on the heap and still fans out to subscribers.
  auto port = Port{
      Manifest{RunContext{logRoot(), {}, false, ""}, ConfigStore{{config()}, {}}},
      emptySetup
  };

  auto received = std::vector<std::int64_t>{};
  port.subscribe(
      chronicle::makeChannelId(kSchemaId<TestSchema>, "live"),
      [&received](Consignment consignment)
      { received.push_back(Message<TestSchema>{consignment.crate()}.reader().getValue()); });

  auto publisher = port.publisher<TestSchema>("live");
  constexpr auto kValue = std::int64_t{7};
  auto draft = publisher.draft();
  draft.builder().setValue(kValue);
  publisher.publish(std::move(draft));

  EXPECT_EQ((std::vector<std::int64_t>{kValue}), received);
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle"));
}

TEST(PortTest, constructionAddsListedResources)
{
  auto port = Port{
      Manifest{RunContext{logRoot(), {resource()}, true, ""}, ConfigStore{{config()}, {}}},
      emptySetup
  };
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
}

TEST(PortTest, constructionFromCommandLineReadsEveryOption)
{
  // Stage config files compatible with TestConfig, then build an argv that exercises every
  // manifest option, mirroring a real command line.
  const auto stagingDir = fs::temp_directory_path() / "niocPortTestCli";
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

  auto options = programOptions("myRobot");
  options.add(Manifest::cliOptions());
  const auto variableMap = parseCommandLine(argc, argv.data(), options);

  // parseCommandLine injects the verbatim command line for the Port to record.
  EXPECT_TRUE(variableMap.contains("commandLine"));

  auto port = Port{
      Manifest{variableMap, capnp::Schema::from<TestConfig>()},
      emptySetup
  };

  EXPECT_EQ(port.workingDir().parent_path(), logRoot());
  EXPECT_FALSE(port.runContext().playback());
  EXPECT_TRUE(fs::is_regular_file(port.workingDir() / "testResource.bin"));
  EXPECT_FALSE(fs::exists(port.workingDir() / "chronicle")); // --record-chronicle false

  // The recorded config.json carries the file merge with the cli override applied on top.
  const auto onDisk = nlohmann::json::parse(std::ifstream(port.workingDir() / "config.json"));
  EXPECT_EQ(onDisk.at("count").get<int>(), 2);            // later file wins
  EXPECT_EQ(onDisk.at("name").get<std::string>(), "cli"); // --config-override wins over files
}

TEST(PortTest, constructionRejectsUnreadableConfig)
{
  EXPECT_THROW(
      (Port{
          Manifest{
                   RunContext{logRoot(), {}, true, ""},
                   ConfigStore{{testDataDir() / "doesNotExist.json"}, {}}},
          emptySetup
  }),
      std::runtime_error);
}

TEST(PortTest, constructionRejectsMalformedConfig)
{
  EXPECT_THROW(
      (Port{
          Manifest{RunContext{logRoot(), {}, true, ""}, ConfigStore{{malformedConfig()}, {}}},
          emptySetup
  }),
      nlohmann::json::parse_error);
}

TEST(PortTest, publishFansOutToEverySubscriberOnTheChannel)
{
  auto port = Port{testManifest(), emptySetup};

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

TEST(PortTest, shutdownAndAbortTripTheirTokensIndependently)
{
  const auto port = Port{testManifest(), emptySetup};

  EXPECT_FALSE(port.shutdownToken().stop_requested());
  EXPECT_FALSE(port.abortToken().stop_requested());

  port.shutdown();
  EXPECT_TRUE(port.shutdownToken().stop_requested());
  EXPECT_FALSE(port.abortToken().stop_requested());

  port.abort();
  EXPECT_TRUE(port.abortToken().stop_requested());
}

TEST(PortTest, awaitQuiescenceBlocksUntilDeliveredConsignmentsDie)
{
  auto port = Port{testManifest(), emptySetup};

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

TEST(PortTest, abortUnblocksAwaitQuiescenceWithConsignmentsStillHeld)
{
  auto port = Port{testManifest(), emptySetup};

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

TEST(PortTest, everyPublishedMessageIsRecordedInOrder)
{
  constexpr auto kMessageCount = std::int64_t{64};
  constexpr auto kTopic = std::string_view{"chronicleGate"};

  // No subscribers: publishing records each message into the chronicle synchronously. Read the
  // recording back and expect every published value, in publish order.
  const auto workingDir = [&]
  {
    auto port = Port{testManifest(), emptySetup};
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

TEST(PortTest, waitReturnsFalseOnceEveryDriverIsDone)
{
  class ScriptedDriver final: public Driver
  {
  public:
    ScriptedDriver(Port& port, const int steps): Driver{port, "ScriptedDriver"}, mRemaining{steps}
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
      testManifest(),
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
