////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace naksh::messages
{

class MessageBase
{
public:
    using MessageId = uint64_t;

    MessageBase() = default;

    virtual ~MessageBase() = default;

    [[nodiscard]] virtual MessageId messageId() const = 0;

private:

};


} // End of namespace naksh::messages.
