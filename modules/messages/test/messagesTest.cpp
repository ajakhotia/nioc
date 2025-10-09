////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/chronicle/chronicle.hpp>
#include <nioc/messages/idl/sample1.capnp.h>
#include <nioc/messages/msg.hpp>

namespace nioc::messages
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

TEST(MessagesTest, ChronicleWriterReader)
{
  std::filesystem::path chroniclePath;
  {
    chronicle::Writer writer;
    chroniclePath = writer.path();

    Msg<Sample1> sampleMsg;
    {
      auto builder = sampleMsg.builder();
      builder.setName("example");
      builder.setValue(3);
    }
    write(sampleMsg, writer);
  }

  {
    auto reader = chronicle::Reader(chroniclePath);
    auto entry = reader.read();
    auto msg = Msg<Sample1>(entry.mMemoryCrate);
    auto msgReader = msg.reader();

    EXPECT_EQ(entry.mChannelId, Msg<Sample1>::kMsgHandle);
    EXPECT_EQ("example", msgReader.getName());
    EXPECT_EQ(3, msgReader.getValue());
  }
}


} // namespace nioc::messages
