////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/logger/logger.hpp>
#include <naksh/logger/logReader.hpp>
#include <naksh/messages/idl/sample1.capnp.h>
#include <naksh/messages/msg.hpp>

namespace naksh::messages
{
TEST(MessagesTest, MsgHandleChecks)
{
    static_assert(Msg<Sample1>::kMsgHandle == Sample1::_capnpPrivate::typeId);

    const Msg<Sample1> testMsg;
    EXPECT_EQ(testMsg.msgHandle(), Msg<Sample1>::kMsgHandle);

    const auto& baseRef = dynamic_cast<const MsgBase&>(testMsg);
    EXPECT_EQ(baseRef.msgHandle(), Msg<Sample1>::kMsgHandle);
}


TEST(MessagesTest, Construction)
{
    Msg<Sample1> sampleMsg;
    {
        auto builder = sampleMsg.builder();
        builder.setName("example");
        builder.setValue(3);
    }

    {
        auto reader = sampleMsg.reader();
        EXPECT_EQ("example", reader.getName());
        EXPECT_EQ(3, reader.getValue());
    }
}


TEST(MessagesTest, Logger)
{
    std::filesystem::path logPath;
    {
        logger::Logger logger;
        logPath = logger.path();

        Msg<Sample1> sampleMsg;
        {
            auto builder = sampleMsg.builder();
            builder.setName("example");
            builder.setValue(3);
        }
        write(sampleMsg, logger);
    }

    {
        auto logReader = logger::LogReader(logPath);
        auto logEntry = logReader.read();
        auto msg = Msg<Sample1>(logEntry.mMemoryCrate);
        auto reader = msg.reader();

        EXPECT_EQ(logEntry.mChannelId, Msg<Sample1>::kMsgHandle);
        EXPECT_EQ("example", reader.getName());
        EXPECT_EQ(3, reader.getValue());
    }
}


} // namespace naksh::messages

#pragma clang diagnostic pop
