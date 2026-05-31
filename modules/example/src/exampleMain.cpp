////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <memory>
#include <nioc/example/exampleComponent1.hpp>
#include <nioc/example/exampleComponent2.hpp>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/threadedRunner.hpp>

int main()
{
  constexpr auto kInboxCapacity = std::size_t{ 16 };
  constexpr auto kDriverRounds = std::size_t{ 8 };

  auto port = nioc::terminus::Port{};

  // ExampleDriver produces Sample1 and Sample3.
  const auto exampleDriver =
      std::make_shared<nioc::example::ExampleDriver>(port, "sample1", "sample3", kDriverRounds);

  // ExampleComponent1 consumes Sample3 and produces Sample2.
  const auto exampleComponent1 = std::make_shared<nioc::example::ExampleComponent1>(
      port,
      "sample3",
      "sample2",
      kInboxCapacity,
      nioc::terminus::OverflowPolicy::Overwrite);

  // ExampleComponent2 consumes Sample1, Sample2, and Sample3 and logs the running counts.
  const auto exampleComponent2 = std::make_shared<nioc::example::ExampleComponent2>(
      port,
      "sample1",
      "sample2",
      "sample3",
      kInboxCapacity,
      nioc::terminus::OverflowPolicy::Overwrite);

  const auto component1Runner = std::make_shared<nioc::terminus::ThreadedRunner>();
  component1Runner->launch(exampleComponent1);

  const auto component2Runner = std::make_shared<nioc::terminus::ThreadedRunner>();
  component2Runner->launch(exampleComponent2);

  const auto driverRunner = std::make_shared<nioc::terminus::ThreadedRunner>();
  driverRunner->launch(exampleDriver);

  driverRunner->waitUntilStopped();

  component1Runner->requestStop();
  component1Runner->waitUntilStopped();

  component2Runner->requestStop();
  component2Runner->waitUntilStopped();

  return EXIT_SUCCESS;
}
