////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace naksh::example
{

class Example
{
public:
    /// Default constructor.
    Example();

    /// Constructor
    explicit Example(std::string name);

    /// Default copy constructor.
    Example(const Example&) = default;

    /// Default move constructor.
    Example(Example&&) = default;

    /// Default destructor.
    ~Example() = default;

    /// Default copy assignment operator.
    Example& operator=(const Example&) = default;

    /// Default move assignment operator.
    Example& operator=(Example&&) = default;

    [[nodiscard]] const std::string& name() const noexcept;

private:
    std::string mName;
};

} // namespace naksh::example
