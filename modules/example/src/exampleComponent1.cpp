////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/exampleComponent1.hpp>
#include <utility>

namespace nioc::example
{

ExampleComponent1::ExampleComponent1(
    terminus::Port& port,
    const ExampleComponent1Config::Reader config):
  Component{port, config.getComponent()},
  mSample2Publisher{publisher<Sample2>(config.getSample2Topic().cStr())}
{
  subscribe<Sample3>(
      config.getSample3Topic().cStr(),
      [this](const terminus::Message<Sample3>& message)
      {
        process(message);
        return State::Continue;
      });
}

void ExampleComponent1::process(const terminus::Message<Sample3>& /* message */)
{
  auto sample2 = mSample2Publisher.draft();
  auto builder = sample2.builder();
  builder.setName("fromExampleComponent1");
  builder.setIdNumber(0);
  builder.setRank(0);
  mSample2Publisher.publish(std::move(sample2));
}

} // namespace nioc::example
