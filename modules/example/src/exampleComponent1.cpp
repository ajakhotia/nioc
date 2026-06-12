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
    const ExampleComponent1Config::Reader config):
  Component{port, config.getComponent()},
  mSample3Topic{config.getSample3Topic().cStr()},
  mSample2Topic{config.getSample2Topic().cStr()}
{
  subscribe<Sample3>(
      mSample3Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
        return State::Continue;
      });
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
