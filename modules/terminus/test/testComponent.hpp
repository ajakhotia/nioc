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

class EarthComponent final: public Component
{
public:
  static constexpr std::string_view kTopic{"earth"};

  EarthComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

class MarsComponent final: public Component
{
public:
  static constexpr std::string_view kTopic{"mars"};

  MarsComponent(Port& port, std::size_t inboxCapacity, concurrent::BufferMode bufferMode);

  [[nodiscard]] std::size_t stepCount() const;

private:
  std::atomic<std::size_t> mStepCount{0};
};

} // namespace nioc::terminus
