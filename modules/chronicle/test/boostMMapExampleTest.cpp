////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/iostreams/device/mapped_file.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace nioc::chronicle
{
namespace
{
constexpr const auto kNumBytes = 1024ULL * 1024ULL;

} // namespace

TEST(LoggerBoostUsageExample, MMapFileWriting)
{
  namespace bio = boost::iostreams;
  std::filesystem::remove_all("/tmp/mmapFileWriting.txt");

  auto mappedFileParams = bio::mapped_file_params{};
  mappedFileParams.new_file_size = kNumBytes;
  mappedFileParams.path = "/tmp/mmapFileWriting.txt";
  mappedFileParams.flags = bio::mapped_file::mapmode::readwrite;

  auto file = bio::mapped_file{mappedFileParams};

  const auto tic = std::clock();
  {
    for(auto& byte: file)
    {
      byte = 'a';
    }

    file.close();
  }
  const auto toc = std::clock();
  const auto duration = toc - tic;
  std::filesystem::remove("/tmp/mmapFileWriting.txt");
  std::cout
      << "MMap write time: " << static_cast<double>(duration) / static_cast<double>(CLOCKS_PER_SEC)
      << '\n';
}

TEST(LoggerBoostUsageExample, SerialFileWrite)
{
  std::filesystem::remove_all("/tmp/serialFileWriting.txt");

  // Create a file and  write 'a' to it.
  auto file = std::ofstream{"/tmp/serialFileWriting.txt"};

  const auto tic = std::clock();
  {
    for(auto i = 0ULL; i < kNumBytes; ++i)
    {
      file << 'a';
    }

    file.close();
  }
  const auto toc = std::clock();
  const auto duration = toc - tic;
  std::filesystem::remove("/tmp/serialFileWriting.txt");
  std::cout << "Serial write time: "
            << static_cast<double>(duration) / static_cast<double>(CLOCKS_PER_SEC) << '\n';
}


} // namespace nioc::chronicle
