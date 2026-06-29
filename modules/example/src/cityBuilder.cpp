////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/cityBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <utility>

namespace nioc::example
{

CityBuilder::CityBuilder(terminus::Port& port, const CityBuilderConfig::Reader config):
  Component{port, config.getComponent()},
  mConfig{config},
  mCityPublisher{publisher<City>(config.getCityTopic().cStr())}
{
  subscribe<Settlement>(
      config.getSettlementTopic().cStr(),
      [this](const terminus::Message<Settlement>& settlement)
      {
        process(settlement);
        return State::Continue;
      });

  subscribe<Ore>(
      config.getOreTopic().cStr(),
      [this](const terminus::Message<Ore>& ore)
      {
        process(ore);
        return State::Continue;
      });

  subscribe<Grain>(
      config.getGrainTopic().cStr(),
      [this](const terminus::Message<Grain>& grain)
      {
        process(grain);
        return State::Continue;
      });
}

void CityBuilder::process(const terminus::Message<Settlement>& settlement)
{
  logger::info("[{}] received Settlement #{}", name(), settlement.reader().getId());
  ++mSettlementsAvailable;
  build();
}

void CityBuilder::process(const terminus::Message<Ore>& ore)
{
  logger::info("[{}] received Ore #{}", name(), ore.reader().getId());
  ++mOreAvailable;
  build();
}

void CityBuilder::process(const terminus::Message<Grain>& grain)
{
  logger::info("[{}] received Grain #{}", name(), grain.reader().getId());
  ++mGrainAvailable;
  build();
}

void CityBuilder::build()
{
  const auto settlementNeededPerCity = mConfig.getSettlementPerCity();
  const auto oreNeededPerCity = mConfig.getOrePerCity();
  const auto grainNeededPerCity = mConfig.getGrainPerCity();
  while(mSettlementsAvailable >= settlementNeededPerCity and
        mOreAvailable >= oreNeededPerCity and
        mGrainAvailable >= grainNeededPerCity)
  {
    mSettlementsAvailable -= settlementNeededPerCity;
    mOreAvailable -= oreNeededPerCity;
    mGrainAvailable -= grainNeededPerCity;
    const auto cityId = ++mNextCityId;

    auto city = mCityPublisher.draft();
    auto builder = city.builder();
    builder.setId(cityId);
    builder.setBuilder(name());
    logger::info("[{}] publishing City #{}", name(), cityId);
    mCityPublisher.publish(std::move(city));
  }
}

} // namespace nioc::example
