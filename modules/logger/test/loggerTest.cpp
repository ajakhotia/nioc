////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/logger/logger.hpp>
#include <numeric>

namespace naksh::logger
{
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
    EXPECT_NO_THROW(Logger logger("/tmp/foo0x52692", 1024UL * 1024UL));
}


TEST(Logger, writeSpan)
{
    const auto channelA = 16983UL;
    const auto channelB = 68964786UL;

    const auto data = generateData();
    Logger logger(Logger::kDefaultLogPath, 256UL);

    logger.write(channelA, std::as_bytes(std::span(data)));
    logger.write(channelB, std::as_bytes(std::span(data)));
}


TEST(Logger, writeCollectionOfSpan) {}


TEST(Logger, path)
{
    constexpr auto location = "/tmp/foo0x246q46q/";
    Logger logger(location);

    EXPECT_TRUE(logger.path().string().starts_with(location));
}

} // namespace naksh::logger

#pragma clang diagnostic pop
