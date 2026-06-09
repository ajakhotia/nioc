////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <csignal>
#include <cstdlib>
#include <memory>
#include <nioc/common/signalCatcher.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/example/exampleComponent1.hpp>
#include <nioc/example/exampleComponent2.hpp>
#include <nioc/example/exampleDriver.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/port.hpp>

int main()
{
  constexpr auto kInboxCapacity = std::size_t{16};
  constexpr auto kDriverRounds = std::size_t{1000000};

  // Install the default logger so the terminus diagnostic traces reach the console. Its level
  // defaults to the compile-time floor (SPDLOG_ACTIVE_LEVEL), so a trace-floor build prints traces.
  nioc::logger::setupDefaultLogger("exampleMain");

  auto port = nioc::terminus::Port{};

  const auto signalCatcher = nioc::common::SignalCatcher{
      std::pair(
          SIGINT,
          [&port](const std::uint32_t count)
          {
            if(count == 1)
            {
              port.shutdown();
            }
            if(count >= 2)
            {
              port.abort();
            }
          }),
    std::pair(
          SIGTERM,
          [&port](const std::uint32_t count)
          {
            if(count == 1)
            {
              port.shutdown();
            }
            if(count >= 2)
            {
              port.abort();
            }
          }),
      std::pair(
          SIGABRT,
          [&port](const std::uint32_t count)
          {
            if(count >= 1)
            {
              port.abort();
            }
          }),
  };

  // ExampleDriver produces Sample1 and Sample3.
  const auto exampleDriver =
      std::make_shared<nioc::example::ExampleDriver>(port, "sample1", "sample3", kDriverRounds);

  // ExampleComponent1 consumes Sample3 and produces Sample2.
  const auto exampleComponent1 = std::make_shared<nioc::example::ExampleComponent1>(
      port,
      "sample3",
      "sample2",
      kInboxCapacity,
      nioc::concurrent::BufferMode::Unbounded);

  // ExampleComponent2 consumes Sample1, Sample2, and Sample3 and logs the running counts.
  const auto exampleComponent2 = std::make_shared<nioc::example::ExampleComponent2>(
      port,
      "sample1",
      "sample2",
      "sample3",
      kInboxCapacity,
      nioc::concurrent::BufferMode::Unbounded);

  const auto component1Runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
  component1Runner->launch(exampleComponent1);

  const auto component2Runner = std::make_shared<nioc::concurrent::ThreadedRunner>();
  component2Runner->launch(exampleComponent2);

  const auto driverRunner = std::make_shared<nioc::concurrent::ThreadedRunner>();
  driverRunner->launch(exampleDriver);

  driverRunner->waitUntilStopped();

  component1Runner->requestStop();
  component1Runner->waitUntilStopped();

  component2Runner->requestStop();
  component2Runner->waitUntilStopped();

  return EXIT_SUCCESS;
}
