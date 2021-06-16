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
