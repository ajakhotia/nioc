////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <channel.hpp>
#include <gtest/gtest.h>
#include <numeric>
#include <utils.hpp>

namespace nioc::logger
{
namespace fs = std::filesystem;

namespace
{

constexpr auto kDataSize = 20;
constexpr auto kMaxFileSizeInBytes = 256;
constexpr auto kDataIotaStart = 65; // Corresponds to character A in ASCII
constexpr auto kNumFramesToWrite = 256UL;

std::vector<char> generateTestDataFrame()
{
  std::vector<char> data(kDataSize);
  std::iota(data.begin(), data.end(), kDataIotaStart);
  return data;
}

} // namespace

TEST(Channel, construction)
{
  const auto testLogDirectoryPath = fs::temp_directory_path() / "niocChannelTest";
  fs::remove_all(testLogDirectoryPath);

  EXPECT_NO_THROW((Channel(testLogDirectoryPath, kMaxFileSizeInBytes)));
  EXPECT_THROW((Channel(testLogDirectoryPath)), std::logic_error);

  fs::remove_all(testLogDirectoryPath);
  EXPECT_NO_THROW((Channel(testLogDirectoryPath)));

  fs::remove_all(testLogDirectoryPath);
}

TEST(Channel, rollAndIndexFileSizeChecks)
{
  const auto testLogDirectoryPath = fs::temp_directory_path() / "niocChannelTest";
  fs::remove_all(testLogDirectoryPath);

  // Create a channel and write frames to it.
  {
    Channel channel(testLogDirectoryPath, kMaxFileSizeInBytes);

    const auto data = generateTestDataFrame();
    for(size_t ii = 0U; ii < kNumFramesToWrite; ++ii)
    {
      channel.writeFrame(std::as_bytes(std::span(data)));
    }
  }

  constexpr auto numFramesPerFullFile = kMaxFileSizeInBytes / kDataSize;
  constexpr auto expectedFullFileSize = numFramesPerFullFile * kDataSize;
  constexpr auto numFramesInLastFile = kNumFramesToWrite % numFramesPerFullFile;
  constexpr auto expectedLastFileSize = numFramesInLastFile * kDataSize;
  constexpr auto expectedIndexFileSize = kNumFramesToWrite * 3 * sizeof(uint64_t);

  std::vector<fs::directory_entry> directoryEntries;
  for(const auto& entity: fs::directory_iterator(testLogDirectoryPath))
  {
    directoryEntries.emplace_back(entity);
  }

  std::ranges::sort(directoryEntries);

  // We expect that the first file in this list to be the index file.
  EXPECT_EQ(directoryEntries.front().path(), testLogDirectoryPath / kIndexFileName);
  EXPECT_EQ(fs::file_size(directoryEntries.front()), expectedIndexFileSize);

  const auto rollSpan = std::span(
      std::next(directoryEntries.begin()),
      std::prev(directoryEntries.end()));

  for(const auto& item: rollSpan)
  {
    EXPECT_EQ(fs::file_size(item), expectedFullFileSize);
  }

  EXPECT_EQ(fs::file_size(directoryEntries.back()), expectedLastFileSize);

  fs::remove_all(testLogDirectoryPath);
}

} // namespace nioc::logger
