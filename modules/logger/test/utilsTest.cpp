////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"

#include <gtest/gtest.h>

namespace naksh::logger
{

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


} // namespace naksh::logger
