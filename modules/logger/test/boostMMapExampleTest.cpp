////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace naksh::logger
{
namespace
{

constexpr const long int kNumBytes = 1024 * 1024;

}


TEST(LoggerBoostUsageExample, MMapFileWriting)
{
    namespace bio = boost::iostreams;

    const auto tic = std::clock();
    {
        bio::mapped_file_params mappedFileParams;
        mappedFileParams.new_file_size = kNumBytes;
        mappedFileParams.path = "/tmp/mmapFileWriting.txt";
        mappedFileParams.flags = bio::mapped_file::mapmode::readwrite;

        bio::mapped_file file(mappedFileParams);

        for(auto& byte: file)
        {
            byte = 'a';
        }

        file.close();
    }
    const auto toc = std::clock();
    const auto duration = toc - tic;
    std::filesystem::remove("/tmp/mmapFileWriting.txt");
    std::cout << "MMap write time: " << double(duration) / double(CLOCKS_PER_SEC) << std::endl;
}


TEST(LoggerBoostUsageExample, SerialFileWrite)
{
    // Create a file and  write 'a' to it.
    const auto tic = std::clock();
    {
        std::ofstream file("/tmp/serialFileWriting.txt");

        for(long int i = 0; i < kNumBytes; ++i)
        {
            file << 'a';
        }

        file.close();
    }
    const auto toc = std::clock();
    const auto duration = toc - tic;
    std::filesystem::remove("/tmp/serialFileWriting.txt");
    std::cout << "Serial write time: " << double(duration) / double(CLOCKS_PER_SEC) << std::endl;
}


} // namespace naksh::logger

#pragma clang diagnostic pop
