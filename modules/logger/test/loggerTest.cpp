////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/logger/logger.hpp>

namespace naksh::logger
{

TEST(LoggerTest, ConstructionTest)
{
    Logger logger;
}

} // namespace naksh::logger

#pragma clang diagnostic pop
