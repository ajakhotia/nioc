////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/exampleComponent2.hpp>
#include <nioc/logger/logger.hpp>
#include <utility>

namespace nioc::example
{

ExampleComponent2::ExampleComponent2(
    terminus::Port& port,
    std::string sample1Topic,
    std::string sample2Topic,
    std::string sample3Topic,
    const std::size_t inboxCapacity,
    const concurrent::BufferMode bufferMode):
  Component{port, inboxCapacity, bufferMode, "ExampleComponent2"},
  mSample1Topic{std::move(sample1Topic)},
  mSample2Topic{std::move(sample2Topic)},
  mSample3Topic{std::move(sample3Topic)}
{
  subscribe<Sample1>(
      mSample1Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
        return State::Continue;
      });

  subscribe<Sample2>(
      mSample2Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
        return State::Continue;
      });

  subscribe<Sample3>(
      mSample3Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
        return State::Continue;
      });
}

std::uint64_t ExampleComponent2::sample1Count() const noexcept
{
  return mSample1Count.load();
}

std::uint64_t ExampleComponent2::sample2Count() const noexcept
{
  return mSample2Count.load();
}

std::uint64_t ExampleComponent2::sample3Count() const noexcept
{
  return mSample3Count.load();
}

void ExampleComponent2::process(const terminus::ConstMsgPtr<Sample1>& /* msgPtr */)
{
  mSample1Count.fetch_add(1);
  logCounts();
}

void ExampleComponent2::process(const terminus::ConstMsgPtr<Sample2>& /* msgPtr */)
{
  mSample2Count.fetch_add(1);
  logCounts();
}

void ExampleComponent2::process(const terminus::ConstMsgPtr<Sample3>& /* msgPtr */)
{
  mSample3Count.fetch_add(1);
  logCounts();
}

void ExampleComponent2::logCounts() const
{
  if(constexpr auto kLogEvery = 1000; mSample1Count.load() % kLogEvery == 0)
  {
    logger::info(
        "[{}] counts. Sample1: {}, Sample2: {}, Sample3: {}",
        name(),
        sample1Count(),
        sample2Count(),
        sample3Count());
  }
}

} // namespace nioc::example
