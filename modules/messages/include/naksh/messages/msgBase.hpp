////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

namespace naksh::messages
{

class MsgBase
{
public:
    using MsgHandle = uint64_t;

    virtual ~MsgBase();

    [[nodiscard]] virtual MsgHandle msgHandle() const = 0;

protected:
    MsgBase();
};


} // namespace naksh::messages
