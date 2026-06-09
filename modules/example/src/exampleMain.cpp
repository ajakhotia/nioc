////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <nioc/common/signalCatcher.hpp>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/exampleComponent1.hpp>
#include <nioc/example/exampleComponent2.hpp>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>

int main(const int argC, const char* const* const argV)
{
  constexpr auto kDefaultInboxCapacity = std::size_t{16};
  constexpr auto kDefaultDriverRounds = std::size_t{1000000};

  try
  {
    const auto programName = nioc::common::programName(argC, argV);
    nioc::logger::setupDefaultLogger(programName);

    auto options = nioc::terminus::programOptions(programName);

    // clang-format off
    options.add_options()
    (
      "iterations,iters",
      boost::program_options::value<std::size_t>()->default_value(kDefaultDriverRounds),
      "Number of driver rounds"
    )
    (
      "inbox-capacity,c",
      boost::program_options::value<std::size_t>()->default_value(kDefaultInboxCapacity),
      "Capacity of the inbox"
    );
    // clang-format on

    const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);
    auto port = nioc::terminus::Port{variableMap};

    const auto signalCatcher = nioc::common::SignalCatcher{
        std::pair(
            SIGINT,
            [&port](const std::uint32_t count)
            {
              if(count == 1)
              {
                port.shutdown();
              }
              if(count >= 2)
              {
                port.abort();
              }
            }),
        std::pair(
            SIGTERM,
            [&port](const std::uint32_t count)
            {
              if(count == 1)
              {
                port.shutdown();
              }
              if(count >= 2)
              {
                port.abort();
              }
            }),
        std::pair(
            SIGABRT,
            [&port](const std::uint32_t count)
            {
              if(count >= 1)
              {
                port.abort();
              }
            }),
    };

    // ExampleDriver produces Sample1 and Sample3.
    const auto exampleDriver = std::make_shared<nioc::example::ExampleDriver>(
        port,
        "sample1",
        "sample3",
        variableMap.at("iterations").as<std::size_t>());

    // ExampleComponent1 consumes Sample3 and produces Sample2.
    const auto exampleComponent1 = std::make_shared<nioc::example::ExampleComponent1>(
        port,
        "sample3",
        "sample2",
        variableMap.at("inbox-capacity").as<std::size_t>(),
        nioc::concurrent::BufferMode::Unbounded);

    // ExampleComponent2 consumes Sample1, Sample2, and Sample3 and logs the running counts.
    const auto exampleComponent2 = std::make_shared<nioc::example::ExampleComponent2>(
        port,
        "sample1",
        "sample2",
        "sample3",
        variableMap.at("inbox-capacity").as<std::size_t>(),
        nioc::concurrent::BufferMode::Unbounded);

    const auto component1Runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
    component1Runner->launch(exampleComponent1);

    const auto component2Runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
    component2Runner->launch(exampleComponent2);

    const auto driverRunner = std::make_shared<nioc::concurrent::ThreadedRunner>();
    driverRunner->launch(exampleDriver);
    driverRunner->waitUntilStopped();

    component1Runner->requestStop();
    component1Runner->waitUntilStopped();

    component2Runner->requestStop();
    component2Runner->waitUntilStopped();
  }
  catch(const std::exception& error)
  {
    static_cast<void>(std::fputs("Encountered exception. Error: ", stderr));
    static_cast<void>(std::fputs(error.what(), stderr));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
