////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/logger/channel.hpp>
#include <numeric>

namespace naksh::logger
{

TEST(channelTest, ConstructionTest)
{
    std::filesystem::remove_all("/tmp/testChannel");

    std::vector<char> data(20);
    std::iota(data.begin(), data.end(), 63);

    Channel channel("/tmp/testChannel", 2560);
    for(size_t ii = 0U; ii < 257; ++ii)
    {
        std::cout << "Iter: " << ii << std::endl;
        channel.write(data.size(), data.data());
    }
}

} // namespace naksh::logger

#pragma clang diagnostic pop
