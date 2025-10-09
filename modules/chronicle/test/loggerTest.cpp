////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <gtest/gtest.h>
#include <nioc/chronicle/chronicle.hpp>
#include <numeric>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

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

TEST(Writer, construction)
{
  EXPECT_NO_THROW(Writer writer);
  EXPECT_NO_THROW(Writer writer(fs::temp_directory_path() / "niocUnitTestLogs", 1024UL * 1024UL));
}

TEST(Writer, writeSpan)
{
  constexpr auto channelA = 16983UL;
  constexpr auto channelB = 68964786UL;
  const auto data = generateData();

  const auto logPath = [&]()
  {
    Writer writer;

    writer.write(channelA, std::as_bytes(std::span(data)));
    writer.write(channelB, std::as_bytes(std::span(data)));

    return writer.path();
  }();

  for(const auto& entity: fs::recursive_directory_iterator(logPath))
  {
    if(fs::is_directory(entity))
    {
      continue;
    }

    const auto& entityPathString = entity.path().string();
    if(entityPathString.ends_with(kSequenceFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 16);
    }
    else if(entityPathString.ends_with(kIndexFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 24);
    }
    else if(entityPathString.ends_with(kRollFileNameExtension))
    {
      EXPECT_EQ(fs::file_size(entity), data.size());
    }
    else
    {
      throw std::logic_error("Unexpected file type encountered: " + entityPathString);
    }
  }
}

TEST(Writer, writeCollectionOfSpan)
{
  constexpr auto channelA = 16983UL;
  constexpr auto channelB = 68964786UL;
  const auto data = generateData();

  std::vector<std::span<const std::byte>> spanCollection;
  spanCollection.reserve(10UL);
  for(size_t ii = 0UL; ii < 10UL; ++ii)
  {
    spanCollection.push_back(std::as_bytes(std::span(data)));
  }
  const auto totalSize = computeTotalSizeInBytes(spanCollection);

  const auto logPath = [&]()
  {
    Writer writer;

    writer.write(channelA, spanCollection);
    writer.write(channelB, spanCollection);

    return writer.path();
  }();

  for(const auto& entity: fs::recursive_directory_iterator(logPath))
  {
    if(fs::is_directory(entity))
    {
      continue;
    }

    const auto& entityPathString = entity.path().string();
    if(entityPathString.ends_with(kSequenceFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 16);
    }
    else if(entityPathString.ends_with(kIndexFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 24);
    }
    else if(entityPathString.ends_with(kRollFileNameExtension))
    {
      EXPECT_EQ(fs::file_size(entity), totalSize);
    }
    else
    {
      throw std::logic_error("Unexpected file type encountered: " + entityPathString);
    }
  }
}

TEST(Writer, path)
{
  Writer writer(fs::temp_directory_path() / "niocUnitTestLogs");
  EXPECT_TRUE(writer.path().string().starts_with(
      (fs::temp_directory_path() / "niocUnitTestLogs").string()));
}

} // namespace nioc::chronicle
