////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <iostream>
#include <naksh/logger/channel.hpp>
#include <numeric>

namespace naksh::logger
{
namespace
{
constexpr const auto kTestLogDirectoryName = "/tmp/testChannel0x5832651q";

}


TEST(channelTest, ConstructionTest)
{
    std::filesystem::remove_all(kTestLogDirectoryName);

    std::vector<char> data(20);
    std::iota(data.begin(), data.end(), 63);

    Channel channel(kTestLogDirectoryName, 2560);
    for(size_t ii = 0U; ii < 257; ++ii)
    {
        std::cout << "Iter: " << ii << std::endl;
        channel.write(std::as_bytes(std::span(data)));
    }

    ssize_t inter = 7;
    std::cout << sizeof(inter) << std::endl;
}


} // namespace naksh::logger

#pragma clang diagnostic pop
