////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <naksh/logger/channel.hpp>
#include <naksh/logger/channelReader.hpp>
#include <numeric>

namespace fs = std::filesystem;

namespace naksh::logger
{
namespace
{

const auto kTestChannelDirectoryPath = fs::path("/tmp/testChannel0x558347");
const auto kTestChannelMaxFileSize = 50ULL;
const auto kGeneratedDataSize = 11ULL;
constexpr auto kNumFramesToWrite = 256UL;


std::vector<char> generateTestDataFrame()
{
    std::vector<char> data(kGeneratedDataSize);
    std::iota(data.begin(), data.end(), 0);
    return data;
}

} // namespace

TEST(ChannelReader, construction)
{
    EXPECT_THROW((ChannelReader("/foo")), std::invalid_argument);
}


TEST(ChannelReader, read)
{
    fs::remove_all(kTestChannelDirectoryPath);

    const auto data = generateTestDataFrame();
    const auto dataAsBytes = std::as_bytes(std::span(data));

    // Build a channel on disk.
    {
        auto channel = Channel(kTestChannelDirectoryPath, kTestChannelMaxFileSize);
        for(auto ii = 0ULL; ii < kNumFramesToWrite; ++ii)
        {
            channel.writeFrame(dataAsBytes);
        }
    }

    // Read the built channel
    {
        auto channelReader = ChannelReader(kTestChannelDirectoryPath);
        auto numFramesRead = 0ULL;
        try
        {
            while(true)
            {
                auto crate = channelReader.read();
                ++numFramesRead;

                auto readSpan = crate.span();

                EXPECT_EQ(readSpan.size(), dataAsBytes.size());
                for(auto ii = 0ULL; ii < readSpan.size(); ii++)
                {
                    EXPECT_EQ(readSpan[ii], dataAsBytes[ii]);
                }
            }
        }
        catch(const std::runtime_error& error)
        {
            if(error.what() == std::string("Reached end of file"))
            {
                EXPECT_EQ(numFramesRead, kNumFramesToWrite);
            }
            else
            {
                throw error;
            }
        }
    }

    fs::remove_all(kTestChannelDirectoryPath);
}

} // namespace naksh::logger
