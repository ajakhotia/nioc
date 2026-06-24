////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/exampleDriver.hpp>
#include <utility>

namespace nioc::example
{

ExampleDriver::ExampleDriver(terminus::Port& port, const ExampleDriverConfig::Reader config):
  Driver{port, config.getDriver()},
  mSample1Publisher{publisher<Sample1>(config.getSample1Topic().cStr())},
  mSample3Publisher{publisher<Sample3>(config.getSample3Topic().cStr())}
{
}

auto ExampleDriver::run() -> State
{
  if(shutdownToken().stop_requested())
  {
    return State::Done;
  }

  auto sample1 = mSample1Publisher.draft();
  auto sample1Builder = sample1.builder();
  sample1Builder.setName("sample1");
  sample1Builder.setValue(static_cast<std::int64_t>(mRound));
  mSample1Publisher.publish(std::move(sample1));

  auto sample3 = mSample3Publisher.draft();
  auto sample3Builder = sample3.builder();
  sample3Builder.setLabel("sample3");
  sample3Builder.setFlag(mRound % 2 == 0);
  mSample3Publisher.publish(std::move(sample3));

  ++mRound;
  return State::Continue;
}

} // namespace nioc::example
