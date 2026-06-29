////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/example/config/settlementBuilderConfig.capnp.h>
#include <nioc/example/idl/brick.capnp.h>
#include <nioc/example/idl/grain.capnp.h>
#include <nioc/example/idl/lumber.capnp.h>
#include <nioc/example/idl/road.capnp.h>
#include <nioc/example/idl/settlement.capnp.h>
#include <nioc/example/idl/wool.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief Builds settlements.
///
/// Inputs:
///   - Road
///   - Brick
///   - Lumber
///   - Wool
///   - Grain
///
/// Outputs:
///   - Settlement
///
/// Counts each input as it arrives; once it has a full recipe it spends them and publishes a
/// Settlement. Road is itself the output of the RoadBuilder, so this stage consumes another
/// builder's output. Recipe sizes are read from config each time, so they can be changed while
/// running.
class SettlementBuilder final: public terminus::Component
{
public:
  SettlementBuilder(terminus::Port& port, SettlementBuilderConfig::Reader config);

private:
  SettlementBuilderConfig::Reader mConfig;
  terminus::Publisher<Settlement> mSettlementPublisher;
  std::size_t mRoadsAvailable{0};
  std::size_t mBricksAvailable{0};
  std::size_t mLumberAvailable{0};
  std::size_t mWoolAvailable{0};
  std::size_t mGrainAvailable{0};
  std::uint64_t mNextSettlementId{0};

  /// @brief Take one delivered Road into stock, then build any settlements now affordable.
  void process(const terminus::Message<Road>& road);

  /// @brief Take one delivered Brick into stock, then build any settlements now affordable.
  void process(const terminus::Message<Brick>& brick);

  /// @brief Take one delivered Lumber into stock, then build any settlements now affordable.
  void process(const terminus::Message<Lumber>& lumber);

  /// @brief Take one delivered Wool into stock, then build any settlements now affordable.
  void process(const terminus::Message<Wool>& wool);

  /// @brief Take one delivered Grain into stock, then build any settlements now affordable.
  void process(const terminus::Message<Grain>& grain);

  /// @brief Spend stock into settlements while a full recipe is available.
  void build();
};

} // namespace nioc::example
