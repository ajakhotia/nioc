////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"

#include <gtest/gtest.h>
#include <numeric>

namespace naksh::logger
{
namespace
{

std::vector<char> generateData()
{
    static constexpr auto kSize = 20UL;
    std::vector<char> data(kSize);
    std::iota(data.begin(), data.end(), 63);
    return data;
}

} // namespace


TEST(LoggerUtils, padString)
{
    const auto input = std::string("682");

    {
        const auto output = padString(input, 13U, '0');
        EXPECT_EQ("0000000000682", output);
    }
    {
        const auto output = padString(input, 2U, '0');
        EXPECT_EQ("682", output);
    }
    {
        const auto output = padString(input, 13U, 'Z');
        EXPECT_EQ("ZZZZZZZZZZ682", output);
    }
}


TEST(LoggerUtils, buildRollName)
{
    EXPECT_EQ("roll00000000000000000000.nio", buildRollName(0U));
    EXPECT_EQ("roll00000000000000000001.nio", buildRollName(1U));
    EXPECT_EQ("roll00000003519894239162.nio", buildRollName(3519894239162U));
}


TEST(LoggerUtils, toHexString)
{
    constexpr auto integer = 255U;
    EXPECT_EQ("0xff", toHexString(integer));
}


TEST(LoggerUtils, hexStringToInteger)
{
    constexpr auto hexString = "0xff";
    EXPECT_EQ(255, hexStringToInteger<uint64_t>(hexString));
}



TEST(LoggerUtils, computeTotalSizeInBytes)
{
    const auto data = generateData();
    const auto value = std::as_bytes(std::span(data));
    constexpr auto numRepetitions = 32U;

    const auto spanCollection = std::vector<std::span<const std::byte>>(numRepetitions, value);

    EXPECT_EQ(numRepetitions * data.size(), computeTotalSizeInBytes(spanCollection));
}



TEST(LoggerUtils, ReadWriteUtilSequenceEntry)
{
    std::stringstream stream;
    const auto value = SequenceEntry{53519839189237};

    ReadWriteUtil<SequenceEntry>::write(stream, value);
    const auto readValue = ReadWriteUtil<SequenceEntry>::read(stream.str().data());

    EXPECT_EQ(value.mChannelId, readValue.mChannelId);
}


TEST(LoggerUtils, ReadWriteUtilIndexEntry)
{
    std::stringstream stream;
    const auto value = IndexEntry{53519839189237, 9065316618953, 281591230};

    ReadWriteUtil<IndexEntry>::write(stream, value);
    const auto readValue = ReadWriteUtil<IndexEntry>::read(stream.str().data());

    EXPECT_EQ(value.mRollId, readValue.mRollId);
    EXPECT_EQ(value.mRollPosition, readValue.mRollPosition);
    EXPECT_EQ(value.mDataSize, readValue.mDataSize);
}


TEST(LoggerUtils, ReadWriteByteSpan)
{
    std::stringstream stream;
    const auto data = generateData();
    const auto value = std::as_bytes(std::span(data));
    const auto size = value.size();

    ReadWriteUtil<std::span<const std::byte>>::write(stream, value);
    const auto str = stream.str();
    const auto readValue = ReadWriteUtil<std::span<const std::byte>>::read(str.data(), size);

    EXPECT_TRUE(std::ranges::equal(value, readValue));
}


} // namespace naksh::logger
