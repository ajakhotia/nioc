////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"

namespace naksh::messages
{


template<typename SerializableType_>
class Msg final : public MsgBase
{
public:
    using SerializableType = SerializableType_;

    using SelfType = Msg<SerializableType>;

    using MsgHandle = MsgBase::MsgHandle;

    static constexpr MsgHandle kMsgHandle = SerializableType::_capnpPrivate::typeId;

    [[nodiscard]] MsgHandle msgHandle() const override
    {
        return kMsgHandle;
    }

private:

};

} // End of namespace naksh::messages.
