////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"

namespace nioc::terminus
{

EarthComponent::EarthComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const OverflowPolicy overflowPolicy):
    Component{ port, inboxCapacity, overflowPolicy }
{
}

EarthComponent::State EarthComponent::step()
{
  ++mStepCount;
  return Component::step();
}

std::string_view EarthComponent::name() const
{
  return "EarthComponent";
}

std::size_t EarthComponent::stepCount() const
{
  return mStepCount.load();
}

MarsComponent::MarsComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const OverflowPolicy overflowPolicy):
    Component{ port, inboxCapacity, overflowPolicy }
{
}

MarsComponent::State MarsComponent::step()
{
  ++mStepCount;
  return Component::step();
}

std::string_view MarsComponent::name() const
{
  return "MarsComponent";
}

std::size_t MarsComponent::stepCount() const
{
  return mStepCount.load();
}

} // namespace nioc::terminus
