////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapChannelReader.hpp"
#include "streamChannelWriter.hpp"
#include "utils.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <numeric>

namespace fs = std::filesystem;

namespace nioc::chronicle
{
namespace
{

fs::path testChannelDirectoryPath()
{
  return fs::path{"/tmp/testChannel0x558347"};
}

const auto kTestChannelMaxFileSize = 50ULL;
const auto kGeneratedDataSize = 11ULL;
constexpr auto kNumFramesToWrite = 256UL;

std::vector<char> generateTestDataFrame()
{
  auto data = std::vector<char>(kGeneratedDataSize);
  std::ranges::iota(data, 0);
  return data;
}

} // namespace

TEST(MmapChannelReader, construction)
{
  EXPECT_THROW((MmapChannelReader("/foo")), std::invalid_argument);
}

TEST(MmapChannelReader, read)
{
  const auto channelDirectoryPath = testChannelDirectoryPath();
  fs::remove_all(channelDirectoryPath);

  const auto data = generateTestDataFrame();
  const auto dataAsBytes = std::as_bytes(std::span(data));

  // Build a channel on disk.
  {
    auto channel = StreamChannelWriter(channelDirectoryPath, kTestChannelMaxFileSize);
    for(auto ii = 0ULL; ii < kNumFramesToWrite; ++ii)
    {
      channel.writeFrame(dataAsBytes);
    }
  }

  // Read the built channel
  {
    auto channelReader = MmapChannelReader(channelDirectoryPath);
    auto numFramesRead = 0ULL;
    try
    {
      while(true)
      {
        auto crate = channelReader.read();
        ++numFramesRead;

        auto readSpan = crate.span();

        EXPECT_EQ(readSpan.size(), dataAsBytes.size());
        for(auto ii = 0ULL; ii < readSpan.size(); ++ii)
        {
          EXPECT_EQ(readSpan[ii], dataAsBytes[ii]);
        }
      }
    }
    catch(const std::runtime_error& error)
    {
      if(std::string{error.what()}.ends_with(
             "Reached end of index file at " + (channelDirectoryPath / "index").string()))
      {
        EXPECT_EQ(numFramesRead, kNumFramesToWrite);
      }
      else
      {
        throw;
      }
    }
  }

  fs::remove_all(channelDirectoryPath);
}

} // namespace nioc::chronicle
