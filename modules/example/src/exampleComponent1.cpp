////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <nioc/example/exampleComponent1.hpp>
#include <utility>

namespace nioc::example
{

ExampleComponent1::ExampleComponent1(
    terminus::Port& port,
    std::string sample3Topic,
    std::string sample2Topic,
    const std::size_t inboxCapacity,
    const terminus::OverflowPolicy overflowPolicy):
    Component{ port, inboxCapacity, overflowPolicy },
    mSample3Topic{ std::move(sample3Topic) },
    mSample2Topic{ std::move(sample2Topic) }
{
  subscribe<Sample3>(
      mSample3Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
      });
}

std::string_view ExampleComponent1::name() const
{
  return "ExampleComponent1";
}

void ExampleComponent1::process(const terminus::ConstMsgPtr<Sample3>& /* msgPtr */)
{
  auto sample2 = std::make_shared<terminus::Msg<Sample2>>();
  auto builder = sample2->builder();
  builder.setName("fromExampleComponent1");
  builder.setIdNumber(0);
  builder.setRank(0);
  publish<Sample2>(mSample2Topic, std::move(sample2));
}

} // namespace nioc::example
