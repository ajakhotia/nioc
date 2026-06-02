////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <cstddef>
#include <nioc/terminus/component.hpp>
#include <string_view>

namespace nioc::terminus
{

/// @brief Concrete Component for tests, named "EarthComponent".
///
/// Supplies the step() body a bare Component lacks: it counts each invocation, then runs the
/// inherited Component::step() so the inbox drain behaviour is preserved. @ref stepCount exposes
/// the tally so a test can confirm how often it ran.
class EarthComponent final: public Component
{
public:
  EarthComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] State step() override;

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{ 0 };
};

/// @brief Concrete Component for tests, named "MarsComponent".
///
/// Supplies the step() body a bare Component lacks: it counts each invocation, then runs the
/// inherited Component::step() so the inbox drain behaviour is preserved. @ref stepCount exposes
/// the tally so a test can confirm how often it ran.
class MarsComponent final: public Component
{
public:
  MarsComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] State step() override;

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{ 0 };
};

} // namespace nioc::terminus
