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
#include <string_view>

namespace nioc::example
{

/// @brief Example @ref terminus::Driver that publishes a fixed number of rounds, then finishes.
///
/// A minimal source showing how to write a Driver. Each @ref step publishes one `Sample1` and one
/// `Sample3` message, then advances the round counter; after the configured number of rounds it
/// reports @ref terminus::Routine::State::Done.
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
  ///
  /// @param roundCount Number of rounds to publish before finishing.
  ExampleDriver(
      terminus::Port& port,
      std::string sample1Topic,
      std::string sample3Topic,
      std::size_t roundCount);

  /// @brief Publishes one round of messages, or finishes once every round has been published.
  ///
  /// @return @ref terminus::Routine::State::Continue while rounds remain, or @ref
  /// terminus::Routine::State::Done once the round count is reached.
  [[nodiscard]] terminus::Routine::State step() final;

  /// @brief Returns the human-readable routine name, `"ExampleDriver"`.
  [[nodiscard]] std::string_view name() const final;

private:
  std::string mSample1Topic;
  std::string mSample3Topic;
  std::size_t mRoundCount;
  std::size_t mRound{ 0 };
};

} // namespace nioc::example
