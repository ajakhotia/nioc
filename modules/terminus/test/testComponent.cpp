////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/message.hpp>

namespace nioc::terminus
{

EarthComponent::EarthComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Component{port, inboxCapacity, bufferMode, "EarthComponent"}
{
  subscribe<TestSchema>(
      kTopic,
      [this](const Message<TestSchema>&)
      {
        ++mStepCount;
        return State::Continue;
      });
}

std::size_t EarthComponent::stepCount() const
{
  return mStepCount.load();
}

MarsComponent::MarsComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Component{port, inboxCapacity, bufferMode, "MarsComponent"}
{
  subscribe<TestSchema>(
      kTopic,
      [this](const Message<TestSchema>&)
      {
        ++mStepCount;
        return State::Continue;
      });
}

std::size_t MarsComponent::stepCount() const
{
  return mStepCount.load();
}

} // namespace nioc::terminus
