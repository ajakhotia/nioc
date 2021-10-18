////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "messageBase.hpp"

namespace naksh::messages
{


template<typename SerializableType_>
class Message : public MessageBase
{
public:
    using SerializableType = SerializableType_;

    using MessageId  = MessageBase::MessageId;

    static constexpr const MessageId kMessageId = SerializableType::_capnpPrivate::typeId;

    [[nodiscard]] MessageId messageId() const override
    {
        return kMessageId;
    }

private:

};

} // End of namespace naksh::messages.
