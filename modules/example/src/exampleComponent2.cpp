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
    const terminus::OverflowPolicy overflowPolicy):
    Component{ port, inboxCapacity, overflowPolicy },
    mSample1Topic{ std::move(sample1Topic) },
    mSample2Topic{ std::move(sample2Topic) },
    mSample3Topic{ std::move(sample3Topic) }
{
  subscribe<Sample1>(
      mSample1Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
      });

  subscribe<Sample2>(
      mSample2Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
      });

  subscribe<Sample3>(
      mSample3Topic,
      [this](const auto& msgPtr)
      {
        process(msgPtr);
      });
}

std::string_view ExampleComponent2::name() const
{
  return "ExampleComponent2";
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
  logger::info("ExampleComponent2 received Sample1. Count: {}", mSample1Count.fetch_add(1) + 1);
}

void ExampleComponent2::process(const terminus::ConstMsgPtr<Sample2>& /* msgPtr */)
{
  logger::info("ExampleComponent2 received Sample2. Count: {}", mSample2Count.fetch_add(1) + 1);
}

void ExampleComponent2::process(const terminus::ConstMsgPtr<Sample3>& /* msgPtr */)
{
  logger::info("ExampleComponent2 received Sample3. Count: {}", mSample3Count.fetch_add(1) + 1);
}

} // namespace nioc::example
