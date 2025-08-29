////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"
#include <gtest/gtest.h>
#include <nioc/logger/logger.hpp>
#include <nioc/logger/logReader.hpp>
#include <numeric>

namespace nioc::logger
{
namespace fs = std::filesystem;

namespace
{
std::vector<char> generateData(std::uint64_t size)
{
    std::vector<char> data(size);
    std::iota(data.begin(), data.end(), size);
    return data;
}

constexpr const auto channelA = 16983UL;
constexpr const auto channelB = 68964786UL;
const auto dataA = generateData(20ULL);
const auto dataB = generateData(34ULL);
const auto dataAAsBytes = std::as_bytes(std::span(dataA));
const auto dataBAsBytes = std::as_bytes(std::span(dataB));

fs::path createLog()
{
    Logger logger;

    logger.write(channelA, dataAAsBytes);
    logger.write(channelB, dataBAsBytes);
    logger.write(channelA, dataAAsBytes);
    logger.write(channelB, dataBAsBytes);

    return logger.path();
}

void expectSpanEqual(const std::span<const std::byte>& lhs, const std::span<const std::byte>& rhs)
{
    EXPECT_EQ(lhs.size(), rhs.size());
    for(auto ii = 0ULL; ii < lhs.size(); ++ii)
    {
        EXPECT_EQ(lhs[ii], rhs[ii]);
    }
}

} // namespace


TEST(LogReader, read)
{
    const auto logPath = createLog();
    LogReader logReader(logPath);

    {
        const auto logEntry = logReader.read();
        EXPECT_EQ(channelA, logEntry.mChannelId);
        expectSpanEqual(dataAAsBytes, logEntry.mMemoryCrate.span());
    }

    {
        const auto logEntry = logReader.read();
        EXPECT_EQ(channelB, logEntry.mChannelId);
        expectSpanEqual(dataBAsBytes, logEntry.mMemoryCrate.span());
    }

    {
        const auto logEntry = logReader.read();
        EXPECT_EQ(channelA, logEntry.mChannelId);
        expectSpanEqual(dataAAsBytes, logEntry.mMemoryCrate.span());
    }

    {
        const auto logEntry = logReader.read();
        EXPECT_EQ(channelB, logEntry.mChannelId);
        expectSpanEqual(dataBAsBytes, logEntry.mMemoryCrate.span());
    }

    EXPECT_THROW(logReader.read(), std::runtime_error);
}


} // namespace nioc::logger

#pragma clang diagnostic pop
