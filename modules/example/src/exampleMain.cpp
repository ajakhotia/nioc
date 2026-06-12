////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/schema.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/config/exampleMainConfig.capnp.h>
#include <nioc/example/exampleComponent1.hpp>
#include <nioc/example/exampleComponent2.hpp>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/defaultSignalCatcher.hpp>
#include <nioc/terminus/manifest.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <utility>

int main(const int argC, const char* const* const argV)
{
  try
  {
    const auto programName = nioc::common::programName(argC, argV);
    nioc::logger::setupDefaultLogger(programName);

    auto options = nioc::terminus::programOptions(programName);
    options.add(nioc::terminus::Manifest::cliOptions());
    const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);

    // Builds the example graph: two components consuming and one driver producing the sample
    // topics, each driven by its own threaded runner. Every routine reads its own block of the
    // typed config; the schema's defaults run as-is, patched by any `--append-config` file (see
    // config/nioc/example/defaultExample.json) and `--config-override` entries.
    auto manifest = nioc::terminus::Manifest{
        variableMap,
        capnp::Schema::from<nioc::example::ExampleMainConfig>()};

    auto port = nioc::terminus::Port{
        std::move(manifest),
        [](nioc::terminus::Port& port,
           nioc::terminus::Port::Drivers& drivers,
           nioc::terminus::Port::Components& components,
           nioc::terminus::Port::Runners& runners)
        {
          const auto config = port.config<nioc::example::ExampleMainConfig>();

          // ExampleComponent1 consumes Sample3 and produces Sample2.
          components.push_back(std::make_shared<nioc::example::ExampleComponent1>(
              port,
              config.getExampleComponent1()));

          // ExampleComponent2 consumes Sample1, Sample2, and Sample3 and logs the running counts.
          components.push_back(std::make_shared<nioc::example::ExampleComponent2>(
              port,
              config.getExampleComponent2()));

          // ExampleDriver produces Sample1 and Sample3.
          drivers.push_back(
              std::make_shared<nioc::example::ExampleDriver>(port, config.getExampleDriver()));

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
