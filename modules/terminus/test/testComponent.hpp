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

/// @brief Test Component that subscribes to @ref kTopic and counts its @ref step calls.
class EarthComponent final: public Component
{
public:
  /// @brief Topic the component subscribes to on construction.
  static constexpr std::string_view kTopic{"earth"};

  EarthComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

/// @brief Test Component that subscribes to @ref kTopic and counts its @ref step calls.
class MarsComponent final: public Component
{
public:
  /// @brief Topic the component subscribes to on construction.
  static constexpr std::string_view kTopic{"mars"};

  MarsComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  /// @brief Returns the number of times @ref step has been called.
  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

} // namespace nioc::terminus
