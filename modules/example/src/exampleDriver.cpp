////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/example/idl/sample1.capnp.h>
#include <nioc/example/idl/sample3.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <utility>

namespace nioc::example
{

ExampleDriver::ExampleDriver(
    terminus::Port& port,
    std::string sample1Topic,
    std::string sample3Topic):
  Driver{port, "ExampleDriver"},
  mSample1Topic{std::move(sample1Topic)},
  mSample3Topic{std::move(sample3Topic)}
{
}

auto ExampleDriver::run() -> State
{
  if(shutdownToken().stop_requested())
  {
    return State::Done;
  }

  auto sample1 = std::make_shared<terminus::Msg<Sample1>>();
  auto sample1Builder = sample1->builder();
  sample1Builder.setName("sample1");
  sample1Builder.setValue(static_cast<std::int64_t>(mRound));
  publish<Sample1>(mSample1Topic, std::move(sample1));

  auto sample3 = std::make_shared<terminus::Msg<Sample3>>();
  auto sample3Builder = sample3->builder();
  sample3Builder.setLabel("sample3");
  sample3Builder.setFlag(mRound % 2 == 0);
  publish<Sample3>(mSample3Topic, std::move(sample3));

  ++mRound;
  return State::Continue;
}

} // namespace nioc::example
