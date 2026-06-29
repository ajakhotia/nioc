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

#include <capnp/schema.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <nioc/common/utils.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/cityBuilder.hpp>
#include <nioc/example/config/catanConfig.capnp.h>
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

    // Decode the run's config against CatanConfig: schema defaults, then any --append-config file,
    // then any --config-override entries.
    auto manifest = nioc::terminus::Manifest{
        variableMap,
        capnp::Schema::from<nioc::example::CatanConfig>()};

    // The Port owns the run. Its constructor calls this hook to build the routine graph, handing
    // each routine only its own config block.
    auto port = nioc::terminus::Port{
        std::move(manifest),
        [](nioc::terminus::Port& port,
           nioc::terminus::Port::Drivers& drivers,
           nioc::terminus::Port::Components& components,
           nioc::terminus::Port::Runners& runners)
        {
          const auto config = port.config<nioc::example::CatanConfig>();

          // Components (consumers).
          components.push_back(
              std::make_shared<nioc::example::RoadBuilder>(port, config.getRoadBuilder()));
          components.push_back(
              std::make_shared<nioc::example::SettlementBuilder>(
                  port,
                  config.getSettlementBuilder()));
          components.push_back(
              std::make_shared<nioc::example::CityBuilder>(port, config.getCityBuilder()));
          components.push_back(
              std::make_shared<nioc::example::DevelopmentCardBuilder>(
                  port,
                  config.getDevelopmentCardBuilder()));

          // Drivers (producers).
          drivers.push_back(std::make_shared<nioc::example::Hills>(port, config.getHills()));
          drivers.push_back(std::make_shared<nioc::example::Forest>(port, config.getForest()));
          drivers.push_back(std::make_shared<nioc::example::Pasture>(port, config.getPasture()));
          drivers.push_back(std::make_shared<nioc::example::Fields>(port, config.getFields()));
          drivers.push_back(
              std::make_shared<nioc::example::Mountains>(port, config.getMountains()));

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
