////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/messages/idl/sample1.capnp.h>
#include <naksh/messages/message.hpp>
#include <gtest/gtest.h>

namespace naksh::messages
{

TEST(MessagesTest, MessageIdentityChecks)
{
    static_assert(Message<Sample1>::kMessageId == Sample1::_capnpPrivate::typeId);

    const Message<Sample1> testMsg;
    EXPECT_EQ(testMsg.messageId(), Sample1::_capnpPrivate::typeId);

    const auto& baseRef = dynamic_cast<const MessageBase&>(testMsg);
    EXPECT_EQ(baseRef.messageId(), Sample1::_capnpPrivate::typeId);
}

} // End of namespace naksh::messages.

#pragma clang diagnostic pop
