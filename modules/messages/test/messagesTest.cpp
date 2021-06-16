////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/messages/idl/sample.capnp.h>
#include <naksh/messages/message.hpp>
#include <gtest/gtest.h>

namespace naksh::messages
{

TEST(MessagesTest, MessageIdentityChecks)
{
    static_assert(Message<Sample>::kMessageId == Sample::_capnpPrivate::typeId);

    const Message<Sample> testMsg;
    EXPECT_EQ(testMsg.messageId(), Sample::_capnpPrivate::typeId);

    const auto& baseRef = dynamic_cast<const MessageBase&>(testMsg);
    EXPECT_EQ(baseRef.messageId(), Sample::_capnpPrivate::typeId);
}

} // End of namespace naksh::messages.
