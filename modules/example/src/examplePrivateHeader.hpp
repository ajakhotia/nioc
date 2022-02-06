////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace naksh::example
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

} // namespace naksh::example
