////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <string>

namespace nioc::example
{

/// @brief Example @ref terminus::Driver that publishes a fixed number of rounds, then finishes.
///
/// A minimal source showing how to write a Driver. Each @ref run publishes one `Sample1` and one
/// `Sample3` message, then advances the round counter; after the configured number of rounds it
/// reports @ref concurrent::Routine::State::Done.
class ExampleDriver final: public terminus::Driver
{
public:
  /// @brief Constructs the driver with its output topics and round count.
  ///
  /// @param port Hub the messages are published onto; must outlive this driver.
  ///
  /// @param sample1Topic Topic the `Sample1` messages are published on.
  ///
  /// @param sample3Topic Topic the `Sample3` messages are published on.
  ExampleDriver(terminus::Port& port, std::string sample1Topic, std::string sample3Topic);

private:
  std::string mSample1Topic;
  std::string mSample3Topic;
  std::size_t mRound{0};

  /// @brief Publishes one round of messages or finishes once every round has been published.
  ///
  /// @return @ref concurrent::Routine::State::Continue while rounds remain, or @ref
  /// concurrent::Routine::State::Done once the round count is reached.
  [[nodiscard]] State run() final;
};

} // namespace nioc::example
