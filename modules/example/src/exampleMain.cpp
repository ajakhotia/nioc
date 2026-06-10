////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/exampleComponent1.hpp>
#include <nioc/example/exampleComponent2.hpp>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/defaultSignalCatcher.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <utility>

int main(const int argC, const char* const* const argV)
{
  constexpr auto kDefaultInboxCapacity = std::size_t{16};

  try
  {
    const auto programName = nioc::common::programName(argC, argV);
    nioc::logger::setupDefaultLogger(programName);

    auto options = nioc::terminus::programOptions(programName);

    // clang-format off
    options.add_options()
    (
      "inbox-capacity,c",
      boost::program_options::value<std::size_t>()->default_value(kDefaultInboxCapacity),
      "Capacity of the inbox"
    );
    // clang-format on

    const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);

    // Builds the example graph: two components consuming and one driver producing the sample
    // topics, each driven by its own threaded runner.
    auto port = nioc::terminus::Port{
        variableMap,
        [&variableMap](
            nioc::terminus::Port& port,
            nioc::terminus::Port::Drivers& drivers,
            nioc::terminus::Port::Components& components,
            nioc::terminus::Port::Runners& runners)
        {
          const auto inboxCapacity = variableMap.at("inbox-capacity").as<std::size_t>();

          // ExampleComponent1 consumes Sample3 and produces Sample2.
          components.push_back(std::make_shared<nioc::example::ExampleComponent1>(
              port,
              "sample3",
              "sample2",
              inboxCapacity,
              nioc::concurrent::BufferMode::Unbounded));

          // ExampleComponent2 consumes Sample1, Sample2, and Sample3 and logs the running counts.
          components.push_back(std::make_shared<nioc::example::ExampleComponent2>(
              port,
              "sample1",
              "sample2",
              "sample3",
              inboxCapacity,
              nioc::concurrent::BufferMode::Unbounded));

          // ExampleDriver produces Sample1 and Sample3.
          drivers.push_back(
              std::make_shared<nioc::example::ExampleDriver>(port, "sample1", "sample3"));

          // Consumers launch before the producer so no early message finds its subscriber
          // unscheduled.
          for(const auto& component: components)
          {
            auto runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
            runner->launch(component);
            runners.push_back(std::move(runner));
          }
          for(const auto& driver: drivers)
          {
            auto runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
            runner->launch(driver);
            runners.push_back(std::move(runner));
          }
        }};

    const auto signalCatcher = nioc::terminus::defaultSignalCatcher(port);

    // Keep main parked in fixed beats until the driver finishes; teardown then happens in ~Port:
    // shutdown, quiescence, and destruction of the drivers, components, and runners.
    constexpr auto kPollPeriod = std::chrono::milliseconds{10};
    while(port.wait(kPollPeriod, [] {}))
    {
    }
  }
  catch(const std::exception& error)
  {
    static_cast<void>(std::fputs("Encountered exception. Error: ", stderr));
    static_cast<void>(std::fputs(error.what(), stderr));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
