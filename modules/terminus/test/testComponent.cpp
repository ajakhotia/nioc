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
    const concurrent::BufferMode bufferMode):
  Component{ port, inboxCapacity, bufferMode, "EarthComponent" }
{
}

EarthComponent::State EarthComponent::step()
{
  ++mStepCount;
  return Component::step();
}

std::size_t EarthComponent::stepCount() const
{
  return mStepCount.load();
}

MarsComponent::MarsComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Component{ port, inboxCapacity, bufferMode, "MarsComponent" }
{
}

MarsComponent::State MarsComponent::step()
{
  ++mStepCount;
  return Component::step();
}

std::size_t MarsComponent::stepCount() const
{
  return mStepCount.load();
}

} // namespace nioc::terminus
