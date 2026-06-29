////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/example/config/developmentCardBuilderConfig.capnp.h>
#include <nioc/example/idl/developmentCard.capnp.h>
#include <nioc/example/idl/grain.capnp.h>
#include <nioc/example/idl/ore.capnp.h>
#include <nioc/example/idl/wool.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/publisher.hpp>

namespace nioc::example
{

/// @brief Builds development cards.
///
/// Inputs:
///   - Ore
///   - Wool
///   - Grain
///
/// Outputs:
///   - DevelopmentCard
///
/// Counts each input as it arrives; once it has a full recipe it spends them and publishes a
/// DevelopmentCard. Recipe sizes are read from config each time, so they can be changed while
/// running.
class DevelopmentCardBuilder final: public terminus::Component
{
public:
  DevelopmentCardBuilder(terminus::Port& port, DevelopmentCardBuilderConfig::Reader config);

private:
  DevelopmentCardBuilderConfig::Reader mConfig;
  terminus::Publisher<DevelopmentCard> mDevelopmentCardPublisher;
  std::size_t mOreAvailable{0};
  std::size_t mWoolAvailable{0};
  std::size_t mGrainAvailable{0};
  std::uint64_t mNextDevelopmentCardId{0};

  /// @brief Take one delivered Ore into stock, then buy any cards now affordable.
  void process(const terminus::Message<Ore>& ore);

  /// @brief Take one delivered Wool into stock, then buy any cards now affordable.
  void process(const terminus::Message<Wool>& wool);

  /// @brief Take one delivered Grain into stock, then buy any cards now affordable.
  void process(const terminus::Message<Grain>& grain);

  /// @brief Spend stock into development cards while a full recipe is available.
  void build();
};

} // namespace nioc::example
