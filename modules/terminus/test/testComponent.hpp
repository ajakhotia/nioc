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
/// Subscribes to @ref kTopic on construction with a no-op handler, so a test fills its inbox by
/// publishing on that topic's channel through the Port. Each @ref step counts the invocation, then
/// runs the inherited Component::step to drain the inbox. @ref stepCount exposes the tally so a
/// test can confirm how often it ran.
class EarthComponent final: public Component
{
public:
  /// @brief Topic the component subscribes to on construction; publish here to feed its inbox.
  static constexpr std::string_view kTopic{"earth"};

  EarthComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] State step() override;

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

/// @brief Concrete Component for tests, named "MarsComponent".
///
/// Subscribes to @ref kTopic on construction with a no-op handler, so a test fills its inbox by
/// publishing on that topic's channel through the Port. Each @ref step counts the invocation, then
/// runs the inherited Component::step to drain the inbox. @ref stepCount exposes the tally so a
/// test can confirm how often it ran.
class MarsComponent final: public Component
{
public:
  /// @brief Topic the component subscribes to on construction; publish here to feed its inbox.
  static constexpr std::string_view kTopic{"mars"};

  MarsComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] State step() override;

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

} // namespace nioc::terminus
