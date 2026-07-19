////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/settlementBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

SettlementBuilder::SettlementBuilder(const std::string& name, terminus::Port& port):
  SettlementBuilder{name, port, makeConfig<SettlementBuilderConfig>(port, name)}
{
}

SettlementBuilder::SettlementBuilder(
    std::string name,
    terminus::Port& port,
    terminus::Config<SettlementBuilderConfig> config):
  Component{std::move(name), port, config.reader().getComponent()},
  mConfig{std::move(config)},
  mSettlementPublisher{publisher<Settlement>(mConfig.reader().getSettlementTopic().cStr())}
{
  subscribe<Road>(
      mConfig.reader().getRoadTopic().cStr(),
      [this](const terminus::Message<Road>& road)
      {
        process(road);
        return State::Continue;
      });

  subscribe<Brick>(
      mConfig.reader().getBrickTopic().cStr(),
      [this](const terminus::Message<Brick>& brick)
      {
        process(brick);
        return State::Continue;
      });

  subscribe<Lumber>(
      mConfig.reader().getLumberTopic().cStr(),
      [this](const terminus::Message<Lumber>& lumber)
      {
        process(lumber);
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

void SettlementBuilder::process(const terminus::Message<Road>& road)
{
  logger::info("[{}] received Road #{}", name(), road.reader().getId());
  ++mRoadsAvailable;
  build();
}

void SettlementBuilder::process(const terminus::Message<Brick>& brick)
{
  logger::info("[{}] received Brick #{}", name(), brick.reader().getId());
  ++mBricksAvailable;
  build();
}

void SettlementBuilder::process(const terminus::Message<Lumber>& lumber)
{
  logger::info("[{}] received Lumber #{}", name(), lumber.reader().getId());
  ++mLumberAvailable;
  build();
}

void SettlementBuilder::process(const terminus::Message<Wool>& wool)
{
  logger::info("[{}] received Wool #{}", name(), wool.reader().getId());
  ++mWoolAvailable;
  build();
}

void SettlementBuilder::process(const terminus::Message<Grain>& grain)
{
  logger::info("[{}] received Grain #{}", name(), grain.reader().getId());
  ++mGrainAvailable;
  build();
}

void SettlementBuilder::build()
{
  const auto roadNeededPerSettlement = mConfig.reader().getRoadPerSettlement();
  const auto brickNeededPerSettlement = mConfig.reader().getBrickPerSettlement();
  const auto lumberNeededPerSettlement = mConfig.reader().getLumberPerSettlement();
  const auto woolNeededPerSettlement = mConfig.reader().getWoolPerSettlement();
  const auto grainNeededPerSettlement = mConfig.reader().getGrainPerSettlement();
  while(mRoadsAvailable >= roadNeededPerSettlement and
        mBricksAvailable >= brickNeededPerSettlement and
        mLumberAvailable >= lumberNeededPerSettlement and
        mWoolAvailable >= woolNeededPerSettlement and
        mGrainAvailable >= grainNeededPerSettlement)
  {
    mRoadsAvailable -= roadNeededPerSettlement;
    mBricksAvailable -= brickNeededPerSettlement;
    mLumberAvailable -= lumberNeededPerSettlement;
    mWoolAvailable -= woolNeededPerSettlement;
    mGrainAvailable -= grainNeededPerSettlement;
    const auto settlementId = ++mNextSettlementId;

    auto settlement = mSettlementPublisher.draft();
    auto builder = settlement.builder();
    builder.setId(settlementId);
    builder.setBuilder(name());
    logger::info("[{}] publishing Settlement #{}", name(), settlementId);
    mSettlementPublisher.publish(std::move(settlement));
  }
}

} // namespace nioc::example
