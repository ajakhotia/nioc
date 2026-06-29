////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/example/config/cityBuilderConfig.capnp.h>
#include <nioc/example/idl/city.capnp.h>
#include <nioc/example/idl/grain.capnp.h>
#include <nioc/example/idl/ore.capnp.h>
#include <nioc/example/idl/settlement.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief Builds cities by upgrading a settlement.
///
/// Inputs:
///   - Settlement
///   - Ore
///   - Grain
///
/// Outputs:
///   - City
///
/// Counts each input as it arrives; once it has a full recipe it spends them and publishes a City.
/// Recipe sizes are read from config each time, so they can be changed while running.
class CityBuilder final: public terminus::Component
{
public:
  CityBuilder(terminus::Port& port, CityBuilderConfig::Reader config);

private:
  CityBuilderConfig::Reader mConfig;
  terminus::Publisher<City> mCityPublisher;
  std::size_t mSettlementsAvailable{0};
  std::size_t mOreAvailable{0};
  std::size_t mGrainAvailable{0};
  std::uint64_t mNextCityId{0};

  /// @brief Take one delivered Settlement into stock, then build any cities now affordable.
  void process(const terminus::Message<Settlement>& settlement);

  /// @brief Take one delivered Ore into stock, then build any cities now affordable.
  void process(const terminus::Message<Ore>& ore);

  /// @brief Take one delivered Grain into stock, then build any cities now affordable.
  void process(const terminus::Message<Grain>& grain);

  /// @brief Spend stock into cities while a full recipe is available.
  void build();
};

} // namespace nioc::example
