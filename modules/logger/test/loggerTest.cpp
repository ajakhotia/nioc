////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"
#include <gtest/gtest.h>
#include <nioc/logger/logger.hpp>
#include <numeric>

namespace nioc::logger
{
namespace fs = std::filesystem;

namespace
{

std::vector<char> generateData()
{
    static constexpr auto kSize = 20UL;
    std::vector<char> data(kSize);
    std::iota(data.begin(), data.end(), kSize);
    return data;
}

} // namespace


TEST(Logger, construction)
{
    EXPECT_NO_THROW(Logger logger);
    EXPECT_NO_THROW(
        Logger logger(fs::temp_directory_path() / "niocUnitTestLogs", 1024UL * 1024UL));
}


TEST(Logger, writeSpan)
{
    const auto channelA = 16983UL;
    const auto channelB = 68964786UL;
    const auto data = generateData();

    const auto logPath = [&]()
    {
        Logger logger;

        logger.write(channelA, std::as_bytes(std::span(data)));
        logger.write(channelB, std::as_bytes(std::span(data)));

        return logger.path();
    }();

    for(const auto& entity: fs::recursive_directory_iterator(logPath))
    {
        if(fs::is_directory(entity))
        {
            continue;
        }

        const auto& entityPathString = entity.path().string();
        if(entityPathString.ends_with(kSequenceFileName))
        {
            EXPECT_EQ(fs::file_size(entity), 16);
        }
        else if(entityPathString.ends_with(kIndexFileName))
        {
            EXPECT_EQ(fs::file_size(entity), 24);
        }
        else if(entityPathString.ends_with(kRollFileNameExtension))
        {
            EXPECT_EQ(fs::file_size(entity), data.size());
        }
        else
        {
            throw std::logic_error("Unexpected file type encountered: " + entityPathString);
        }
    }
}


TEST(Logger, writeCollectionOfSpan)
{
    const auto channelA = 16983UL;
    const auto channelB = 68964786UL;
    const auto data = generateData();

    std::vector<std::span<const std::byte>> spanCollection;
    spanCollection.reserve(10UL);
    for(size_t ii = 0UL; ii < 10UL; ++ii)
    {
        spanCollection.push_back(std::as_bytes(std::span(data)));
    }
    const auto totalSize = computeTotalSizeInBytes(spanCollection);

    const auto logPath = [&]()
    {
        Logger logger;

        logger.write(channelA, spanCollection);
        logger.write(channelB, spanCollection);

        return logger.path();
    }();

    for(const auto& entity: fs::recursive_directory_iterator(logPath))
    {
        if(fs::is_directory(entity))
        {
            continue;
        }

        const auto& entityPathString = entity.path().string();
        if(entityPathString.ends_with(kSequenceFileName))
        {
            EXPECT_EQ(fs::file_size(entity), 16);
        }
        else if(entityPathString.ends_with(kIndexFileName))
        {
            EXPECT_EQ(fs::file_size(entity), 24);
        }
        else if(entityPathString.ends_with(kRollFileNameExtension))
        {
            EXPECT_EQ(fs::file_size(entity), totalSize);
        }
        else
        {
            throw std::logic_error("Unexpected file type encountered: " + entityPathString);
        }
    }
}


TEST(Logger, path)
{
    Logger logger(fs::temp_directory_path() / "niocUnitTestLogs");
    EXPECT_TRUE(logger.path().string().starts_with(
        (fs::temp_directory_path() / "niocUnitTestLogs").string()));
}

} // namespace nioc::logger

#pragma clang diagnostic pop
