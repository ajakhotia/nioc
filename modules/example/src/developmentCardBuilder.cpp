////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/developmentCardBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <utility>

namespace nioc::example
{

DevelopmentCardBuilder::DevelopmentCardBuilder(
    terminus::Port& port,
    const DevelopmentCardBuilderConfig::Reader config):
  Component{port, config.getComponent()},
  mConfig{config},
  mDevelopmentCardPublisher{publisher<DevelopmentCard>(config.getDevelopmentCardTopic().cStr())}
{
  subscribe<Ore>(
      config.getOreTopic().cStr(),
      [this](const terminus::Message<Ore>& ore)
      {
        process(ore);
        return State::Continue;
      });

  subscribe<Wool>(
      config.getWoolTopic().cStr(),
      [this](const terminus::Message<Wool>& wool)
      {
        process(wool);
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

void DevelopmentCardBuilder::process(const terminus::Message<Ore>& ore)
{
  logger::info("[{}] received Ore #{}", name(), ore.reader().getId());
  ++mOreAvailable;
  build();
}

void DevelopmentCardBuilder::process(const terminus::Message<Wool>& wool)
{
  logger::info("[{}] received Wool #{}", name(), wool.reader().getId());
  ++mWoolAvailable;
  build();
}

void DevelopmentCardBuilder::process(const terminus::Message<Grain>& grain)
{
  logger::info("[{}] received Grain #{}", name(), grain.reader().getId());
  ++mGrainAvailable;
  build();
}

void DevelopmentCardBuilder::build()
{
  const auto oreNeededPerDevelopmentCard = mConfig.getOrePerCard();
  const auto woolNeededPerDevelopmentCard = mConfig.getWoolPerCard();
  const auto grainNeededPerDevelopmentCard = mConfig.getGrainPerCard();
  while(mOreAvailable >= oreNeededPerDevelopmentCard and
        mWoolAvailable >= woolNeededPerDevelopmentCard and
        mGrainAvailable >= grainNeededPerDevelopmentCard)
  {
    mOreAvailable -= oreNeededPerDevelopmentCard;
    mWoolAvailable -= woolNeededPerDevelopmentCard;
    mGrainAvailable -= grainNeededPerDevelopmentCard;
    const auto developmentCardId = ++mNextDevelopmentCardId;

    auto card = mDevelopmentCardPublisher.draft();
    auto builder = card.builder();
    builder.setId(developmentCardId);
    builder.setBuilder(name());
    logger::info("[{}] publishing DevelopmentCard #{}", name(), developmentCardId);
    mDevelopmentCardPublisher.publish(std::move(card));
  }
}

} // namespace nioc::example
