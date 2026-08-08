////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : ${PROJECT_NAME}
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cadenceMonitor.hpp"
#include <boost/program_options.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/defaultSignalCatcher.hpp>
#include <nioc/terminus/logPlayer.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

/// How often the supervisor loop looks in on the replay.
constexpr auto kPollPeriod = std::chrono::milliseconds{10};

} // namespace

int main(const int argC, const char* const* const argV)
{
  try
  {
    const auto programName = nioc::common::programName(argC, argV);
    nioc::logger::setupDefaultLogger(programName);

    // clang-format off
    auto options = nioc::terminus::RunContext::cliOptions();
    options.add_options()
    (
      "list-topics",
      "List every topic the recording carries as '<topic> <schemaName> <schemaId> <channelId>', "
      "one per line"
    )
    (
      "tally",
      "Replay the recording and print per-topic message counts, rates and periods"
    );
    // clang-format on

    const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, std::move(options));
    const auto listTopics = variableMap.contains("list-topics");
    const auto tally = variableMap.contains("tally");

    if(not variableMap.contains("playback"))
    {
      nioc::common::throwException<std::invalid_argument>(
          "No log was specified for inspection. Pass the log's path with the --playback option.");
    }

    if(listTopics)
    {
      const auto recording = std::filesystem::path{variableMap.at("playback").as<std::string>()};
      std::cout << nioc::terminus::TopicRegistry{recording};
    }

    // Any flag whose answer can only be gathered by replaying the recording. Such flags share one
    // full run: each hangs its own probe off the Port's setup hook below. The probes own their
    // findings and report as the run tears down; main only builds the run and connects the pieces.
    const auto fullRun = tally;
    if(fullRun)
    {
      auto runContext = nioc::terminus::RunContext{variableMap};

      // Read the path out before the context is handed to the Port, which consumes it.
      const auto chronicleDir = runContext.inputLog() / "chronicle";
      nioc::logger::info("Inspecting the log at {}.", chronicleDir.string());

      auto port = nioc::terminus::Port{
          std::move(runContext),
          [&chronicleDir, tally](
              nioc::terminus::Port& port,
              nioc::terminus::Port::Drivers& drivers,
              nioc::terminus::Port::Components& components,
              nioc::terminus::Port::Runners& runners)
          {
            if(tally)
            {
              auto monitor = std::make_shared<nioc::tools::CadenceMonitor>("cadenceMonitor", port);
              auto monitorRunner = std::make_shared<nioc::concurrent::ThreadedRunner>();
              monitorRunner->launch(monitor);

              components.push_back(std::move(monitor));
              runners.push_back(std::move(monitorRunner));
            }

            auto player =
                std::make_shared<nioc::terminus::LogPlayer>("logPlayer", port, chronicleDir);
            auto playerRunner = std::make_shared<nioc::concurrent::ThreadedRunner>();
            playerRunner->launch(player);

            drivers.push_back(std::move(player));
            runners.push_back(std::move(playerRunner));
          }};

      const auto signalCatcher = nioc::terminus::defaultSignalCatcher(port);

      while(port.wait(kPollPeriod, [] {}))
      {
      }
    }
  }
  catch(const std::exception& error)
  {
    // An exception may escape before setupDefaultLogger runs, so report through stderr rather than
    // the logger.
    static_cast<void>(std::fputs("Terminated by an unhandled exception: ", stderr));
    static_cast<void>(std::fputs(error.what(), stderr));
    static_cast<void>(std::fputs("\n", stderr));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
