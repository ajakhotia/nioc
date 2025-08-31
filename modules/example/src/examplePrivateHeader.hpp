////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace nioc::example
{
class PrivateExample
{
public:
  explicit PrivateExample(const int value): mValue(value) {}

  PrivateExample(const PrivateExample&) = default;

  PrivateExample(PrivateExample&&) = default;

  ~PrivateExample() = default;

  PrivateExample& operator=(const PrivateExample&) = default;

  PrivateExample& operator=(PrivateExample&&) = default;

  [[nodiscard]] int value() const noexcept
  {
    return mValue;
  }

private:
  int mValue;
};

} // namespace nioc::example
