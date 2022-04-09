////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <naksh/logger/channel.hpp>
#include <numeric>

namespace naksh::logger
{
namespace fs = std::filesystem;

namespace
{

const auto kTestLogDirectoryPath = fs::path("/tmp/testChannel0x5832651q");
constexpr auto kDataSize = 20;
constexpr auto kMaxFileSizeInBytes = 256;
constexpr auto kDataIotaStart = 65; // Corresponds to character A in ASCII
constexpr auto kNumFramesToWrite = 256UL;


std::vector<char> generateTestDataFrame()
{
    std::vector<char> data(kDataSize);
    std::iota(data.begin(), data.end(), kDataIotaStart);
    return data;
}

} // namespace


TEST(Channel, construction)
{
    fs::remove_all(kTestLogDirectoryPath);

    EXPECT_NO_THROW((Channel(kTestLogDirectoryPath, kMaxFileSizeInBytes)));
    EXPECT_THROW((Channel(kTestLogDirectoryPath)), std::logic_error);

    fs::remove_all(kTestLogDirectoryPath);
    EXPECT_NO_THROW((Channel(kTestLogDirectoryPath)));

    fs::remove_all(kTestLogDirectoryPath);
}


TEST(Channel, rollAndIndexFileSizeChecks)
{
    fs::remove_all(kTestLogDirectoryPath);

    // Create a channel and write frames to it.
    {
        Channel channel(kTestLogDirectoryPath, kMaxFileSizeInBytes);

        const auto data = generateTestDataFrame();
        for(size_t ii = 0U; ii < kNumFramesToWrite; ++ii)
        {
            channel.writeFrame(std::as_bytes(std::span(data)));
        }
    }

    const auto numFramesPerFullFile = kMaxFileSizeInBytes / kDataSize;
    const auto expectedFullFileSize = numFramesPerFullFile * kDataSize;
    const auto numFramesInLastFile = kNumFramesToWrite % numFramesPerFullFile;
    const auto expectedLastFileSize = numFramesInLastFile * kDataSize;
    const auto expectedIndexFileSize = kNumFramesToWrite * 3 * sizeof(uint64_t);

    std::vector<fs::directory_entry> directoryEntries;
    for(const auto& entity: fs::directory_iterator(kTestLogDirectoryPath))
    {
        directoryEntries.emplace_back(entity);
    }

    std::sort(directoryEntries.begin(), directoryEntries.end());

    // We expect that the first file in this list to be the index file.
    EXPECT_EQ(directoryEntries.front().path(), kTestLogDirectoryPath / kIndexFileName);
    EXPECT_EQ(fs::file_size(directoryEntries.front()), expectedIndexFileSize);

    const auto rollSpan =
        std::span(std::next(directoryEntries.begin()), std::prev(directoryEntries.end()));

    for(const auto& item: rollSpan)
    {
        EXPECT_EQ(fs::file_size(item), expectedFullFileSize);
    }

    EXPECT_EQ(fs::file_size(directoryEntries.back()), expectedLastFileSize);

    fs::remove_all(kTestLogDirectoryPath);
}

} // namespace naksh::logger

#pragma clang diagnostic pop
