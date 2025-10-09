////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace nioc::example
{
/// @brief Example class demonstrating library usage.
///
/// Simple class showing basic patterns used in the nioc library.
class Example
{
public:
  /// @brief Creates example with default name.
  Example();

  /// @brief Creates example with custom name.
  /// @param name Name for the example.
  explicit Example(std::string name);

  Example(const Example&) = default;

  Example(Example&&) = default;

  ~Example() = default;

  Example& operator=(const Example&) = default;

  Example& operator=(Example&&) = default;

  /// @brief Gets the example name.
  /// @return Name string.
  [[nodiscard]] const std::string& name() const noexcept;

private:
  std::string mName;
};

} // namespace nioc::example
