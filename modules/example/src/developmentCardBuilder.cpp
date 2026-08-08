////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/developmentCardBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

DevelopmentCardBuilder::DevelopmentCardBuilder(const std::string& name, terminus::Port& port):
  DevelopmentCardBuilder{name, port, port.materializeConfig<DevelopmentCardBuilderConfig>(name)}
{
}

DevelopmentCardBuilder::DevelopmentCardBuilder(
    std::string name,
    terminus::Port& port,
    terminus::Config<DevelopmentCardBuilderConfig> config):
  Component{std::move(name), port, config.reader().getComponent()},
  mConfig{std::move(config)},
  mDevelopmentCardPublisher{
      publisher<DevelopmentCard>(mConfig.reader().getDevelopmentCardTopic().cStr())}
{
  subscribe<Ore>(
      mConfig.reader().getOreTopic().cStr(),
      [this](const terminus::Message<Ore>& ore)
      {
        process(ore);
        return State::Continue;
      });

  subscribe<Wool>(
      mConfig.reader().getWoolTopic().cStr(),
      [this](const terminus::Message<Wool>& wool)
      {
        process(wool);
        return State::Continue;
      });

  subscribe<Grain>(
      mConfig.reader().getGrainTopic().cStr(),
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
  const auto oreNeededPerDevelopmentCard = mConfig.reader().getOrePerCard();
  const auto woolNeededPerDevelopmentCard = mConfig.reader().getWoolPerCard();
  const auto grainNeededPerDevelopmentCard = mConfig.reader().getGrainPerCard();
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
