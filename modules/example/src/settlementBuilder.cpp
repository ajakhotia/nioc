////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/settlementBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <utility>

namespace nioc::example
{

SettlementBuilder::SettlementBuilder(
    terminus::Port& port,
    const SettlementBuilderConfig::Reader config):
  Component{port, config.getComponent()},
  mConfig{config},
  mSettlementPublisher{publisher<Settlement>(config.getSettlementTopic().cStr())}
{
  subscribe<Road>(
      config.getRoadTopic().cStr(),
      [this](const terminus::Message<Road>& road)
      {
        process(road);
        return State::Continue;
      });

  subscribe<Brick>(
      config.getBrickTopic().cStr(),
      [this](const terminus::Message<Brick>& brick)
      {
        process(brick);
        return State::Continue;
      });

  subscribe<Lumber>(
      config.getLumberTopic().cStr(),
      [this](const terminus::Message<Lumber>& lumber)
      {
        process(lumber);
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
  const auto roadNeededPerSettlement = mConfig.getRoadPerSettlement();
  const auto brickNeededPerSettlement = mConfig.getBrickPerSettlement();
  const auto lumberNeededPerSettlement = mConfig.getLumberPerSettlement();
  const auto woolNeededPerSettlement = mConfig.getWoolPerSettlement();
  const auto grainNeededPerSettlement = mConfig.getGrainPerSettlement();
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
