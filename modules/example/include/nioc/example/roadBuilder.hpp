////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/example/config/roadBuilderConfig.capnp.h>
#include <nioc/example/idl/brick.capnp.h>
#include <nioc/example/idl/lumber.capnp.h>
#include <nioc/example/idl/road.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief Builds roads.
///
/// Inputs:
///   - Brick
///   - Lumber
///
/// Outputs:
///   - Road
///
/// Counts each input as it arrives; once it has a full recipe it spends them and publishes a Road.
/// Recipe sizes are read from config each time, so they can be changed while running.
class RoadBuilder final: public terminus::Component
{
public:
  RoadBuilder(terminus::Port& port, RoadBuilderConfig::Reader config);

private:
  RoadBuilderConfig::Reader mConfig;
  terminus::Publisher<Road> mRoadPublisher;
  std::size_t mBricksAvailable{0};
  std::size_t mLumberAvailable{0};
  std::uint64_t mNextRoadId{0};

  /// @brief Take one delivered Brick into stock, then build any roads now affordable.
  void process(const terminus::Message<Brick>& brick);

  /// @brief Take one delivered Lumber into stock, then build any roads now affordable.
  void process(const terminus::Message<Lumber>& lumber);

  /// @brief Spend stock into roads while a full recipe is available.
  void build();
};

} // namespace nioc::example
