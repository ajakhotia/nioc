////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/messages/idl/sample1.capnp.h>
#include <naksh/messages/msg.hpp>

namespace naksh::messages
{
TEST(MessagesTest, MessageIdentityChecks)
{
    static_assert(Msg<Sample1>::kMsgHandle == Sample1::_capnpPrivate::typeId);

    const Msg<Sample1> testMsg;
    EXPECT_EQ(testMsg.msgHandle(), Msg<Sample1>::kMsgHandle);

    const auto& baseRef = dynamic_cast<const MsgBase&>(testMsg);
    EXPECT_EQ(baseRef.msgHandle(), Msg<Sample1>::kMsgHandle);
}

} // namespace naksh::messages

#pragma clang diagnostic pop
