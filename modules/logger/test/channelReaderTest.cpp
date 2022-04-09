////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include "utils.hpp"

#include <gtest/gtest.h>
#include <naksh/logger/channelReader.hpp>

namespace naksh::logger
{

TEST(ChannelReader, construction)
{
    EXPECT_THROW((ChannelReader("/foo")), std::invalid_argument);
}

} // namespace naksh::logger
