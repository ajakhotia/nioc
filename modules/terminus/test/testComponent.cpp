////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "testComponent.hpp"
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>

namespace nioc::terminus
{

EarthComponent::EarthComponent(
    Port& port,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Component{port, inboxCapacity, bufferMode, "EarthComponent"}
{
  subscribe<TestSchema>(kTopic, [](const ConstMsgPtr<TestSchema>&) {});
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
  Component{port, inboxCapacity, bufferMode, "MarsComponent"}
{
  subscribe<TestSchema>(kTopic, [](const ConstMsgPtr<TestSchema>&) {});
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
