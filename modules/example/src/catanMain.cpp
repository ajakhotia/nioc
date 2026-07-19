////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// A complete nioc application in one file. It creates the producers and consumers, connects them
// through a Port (nioc's message bus), and runs until you press Ctrl-C.
//
// clang-format off
//   Producers            Builders (consume -> produce)
//   ---------            -----------------------------
//   hills     -> brick   road builder       : brick, lumber                    -> road
//   forest    -> lumber  settlement builder : road, brick, lumber, wool, grain -> settlement
//   pasture   -> wool    city builder       : settlement, ore, grain           -> city
//   fields    -> grain   dev-card builder   : ore, wool, grain                 -> dev card
//   mountains -> ore
// clang-format on
//
// Producers and consumers share only topic names. The bus delivers each published message to every
// subscriber, so e.g. every grain card reaches all three builders that consume grain. Each finished
// piece is printed by its builder.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/cityBuilder.hpp>
#include <nioc/example/developmentCardBuilder.hpp>
#include <nioc/example/fields.hpp>
#include <nioc/example/forest.hpp>
#include <nioc/example/hills.hpp>
#include <nioc/example/mountains.hpp>
#include <nioc/example/pasture.hpp>
#include <nioc/example/roadBuilder.hpp>
#include <nioc/example/settlementBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/defaultSignalCatcher.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/programOption.hpp>
#include <nioc/terminus/runContext.hpp>
#include <utility>

int main(const int argC, const char* const* const argV)
{
  try
  {
    const auto programName = nioc::common::programName(argC, argV);
    nioc::logger::setupDefaultLogger(programName);

    auto options = nioc::terminus::programOptions(programName);
    options.add(nioc::terminus::RunContext::cliOptions());
    const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);

    // The Port owns the run. Its constructor calls this hook to build the routine graph; each
    // routine reads its own config section (keyed by its name) from the run's config document.
    auto port = nioc::terminus::Port{
        nioc::terminus::RunContext{variableMap},
        [](nioc::terminus::Port& port,
           nioc::terminus::Port::Drivers& drivers,
           nioc::terminus::Port::Components& components,
           nioc::terminus::Port::Runners& runners)
        {
          // Components (consumers).
          components.push_back(
              std::make_shared<nioc::example::RoadBuilder>("rohanTheRoadBuilder", port));
          components.push_back(
              std::make_shared<nioc::example::SettlementBuilder>("sakuraTheSettler", port));
          components.push_back(
              std::make_shared<nioc::example::CityBuilder>("cindyTheCityMaker", port));
          components.push_back(
              std::make_shared<nioc::example::DevelopmentCardBuilder>("deviTheDeveloper", port));

          // Drivers (producers).
          drivers.push_back(std::make_shared<nioc::example::Hills>("hiroHills", port));
          drivers.push_back(std::make_shared<nioc::example::Forest>("finnyForests", port));
          drivers.push_back(std::make_shared<nioc::example::Pasture>("peekyPastures", port));
          drivers.push_back(std::make_shared<nioc::example::Fields>("feiFields", port));
          drivers.push_back(std::make_shared<nioc::example::Mountains>("meiMountains", port));

          // Launch consumers before producers, so no message is published before its subscriber's
          // runner is up. Each routine gets its own thread.
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

    // Park main in fixed beats until shutdown (Ctrl-C); teardown then happens in ~Port.
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
