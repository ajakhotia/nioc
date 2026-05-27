////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/terminus/idl/sample1.capnp.h>
#include <nioc/terminus/msg.hpp>

namespace nioc::terminus
{
namespace
{
/// Create a fresh empty directory at a deterministic path under the system temp
/// directory. Any prior contents are wiped.
std::filesystem::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = std::filesystem::temp_directory_path() / "nioc-terminusTest" / name;
  std::filesystem::remove_all(path);
  std::filesystem::create_directories(path);
  return path;
}
} // namespace

TEST(TerminusTest, MsgHandleChecks)
{
  static_assert(Msg<Sample1>::kMsgHandle == Sample1::_capnpPrivate::typeId);

  const auto testMsg = Msg<Sample1>{};
  EXPECT_EQ(testMsg.msgHandle(), Msg<Sample1>::kMsgHandle);

  const auto& baseRef = dynamic_cast<const MsgBase&>(testMsg);
  EXPECT_EQ(baseRef.msgHandle(), Msg<Sample1>::kMsgHandle);
}

TEST(TerminusTest, Construction)
{
  auto sampleMsg = Msg<Sample1>{};
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

TEST(TerminusTest, ChronicleWriterReader)
{
  const auto chroniclePath = makeFreshEmptyDir("ChronicleWriterReader");
  {
    auto writer = chronicle::Writer{ chroniclePath };

    auto sampleMsg = Msg<Sample1>{};
    {
      auto builder = sampleMsg.builder();
      builder.setName("example");
      builder.setValue(3);
    }
    write(sampleMsg, writer);
  }

  {
    auto reader = chronicle::Reader{ chroniclePath };
    auto entry = reader.read();
    auto msg = Msg<Sample1>{ entry.mMemoryCrate };
    auto msgReader = msg.reader();

    EXPECT_EQ(entry.mChannelId, chronicle::ChannelId(Msg<Sample1>::kMsgHandle));
    EXPECT_EQ("example", msgReader.getName());
    EXPECT_EQ(3, msgReader.getValue());
  }
}


} // namespace nioc::terminus
