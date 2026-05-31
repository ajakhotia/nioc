////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/terminus/driver.hpp>
#include <nioc/terminus/port.hpp>
#include <string>
#include <string_view>

namespace nioc::example
{

class ExampleDriver final: public terminus::Driver
{
public:
  ExampleDriver(
      terminus::Port& port,
      std::string sample1Topic,
      std::string sample3Topic,
      std::size_t roundCount);

  [[nodiscard]] terminus::Routine::State step() final;

  [[nodiscard]] std::string_view name() const final;

private:
  std::string mSample1Topic;
  std::string mSample3Topic;
  std::size_t mRoundCount;
  std::size_t mRound{ 0 };
};

} // namespace nioc::example
