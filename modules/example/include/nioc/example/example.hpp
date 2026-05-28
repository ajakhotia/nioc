////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace nioc::example
{
/// @brief Minimal example class demonstrating the nioc module conventions.
class Example
{
public:
  /// @brief Constructs an Example with a default name.
  Example();

  /// @brief Constructs an Example with the given name.
  /// @param name Name for the example.
  explicit Example(std::string name);

  Example(const Example&) = default;

  Example(Example&&) = default;

  ~Example() = default;

  Example& operator=(const Example&) = default;

  Example& operator=(Example&&) = default;

  /// @brief Returns the example's name.
  [[nodiscard]] const std::string& name() const noexcept;

private:
  std::string mName;
};

} // namespace nioc::example
