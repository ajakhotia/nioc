////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/roadBuilder.hpp>
#include <nioc/logger/logger.hpp>
#include <string>
#include <utility>

namespace nioc::example
{

RoadBuilder::RoadBuilder(const std::string& name, terminus::Port& port):
  RoadBuilder{name, port, makeConfig<RoadBuilderConfig>(port, name)}
{
}

RoadBuilder::RoadBuilder(
    std::string name,
    terminus::Port& port,
    terminus::Config<RoadBuilderConfig> config):
  Component{std::move(name), port, config.reader().getComponent()},
  mConfig{std::move(config)},
  mRoadPublisher{publisher<Road>(mConfig.reader().getRoadTopic().cStr())}
{
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
}

void RoadBuilder::process(const terminus::Message<Brick>& brick)
{
  logger::info("[{}] received Brick #{}", name(), brick.reader().getId());
  ++mBricksAvailable;
  build();
}

void RoadBuilder::process(const terminus::Message<Lumber>& lumber)
{
  logger::info("[{}] received Lumber #{}", name(), lumber.reader().getId());
  ++mLumberAvailable;
  build();
}

void RoadBuilder::build()
{
  const auto brickNeededPerRoad = mConfig.reader().getBrickPerRoad();
  const auto lumberNeededPerRoad = mConfig.reader().getLumberPerRoad();
  while(mBricksAvailable >= brickNeededPerRoad and mLumberAvailable >= lumberNeededPerRoad)
  {
    mBricksAvailable -= brickNeededPerRoad;
    mLumberAvailable -= lumberNeededPerRoad;
    const auto roadId = ++mNextRoadId;

    auto road = mRoadPublisher.draft();
    auto builder = road.builder();
    builder.setId(roadId);
    builder.setBuilder(name());
    logger::info("[{}] publishing Road #{}", name(), roadId);
    mRoadPublisher.publish(std::move(road));
  }
}

} // namespace nioc::example
