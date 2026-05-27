////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <numeric>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

namespace
{

std::vector<char> generateData()
{
  static constexpr auto kSize = 20UL;
  auto data = std::vector<char>(kSize);
  std::iota(data.begin(), data.end(), kSize);
  return data;
}

/// Create a fresh empty directory at a deterministic path under the system temp
/// directory. Any prior contents are wiped. Pass a unique @p name per test to
/// avoid cross-test interference.
fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

} // namespace

TEST(Writer, constructionAcceptsEmptyDirectory)
{
  EXPECT_NO_THROW(Writer writer(makeFreshEmptyDir("ctor-default")));
  EXPECT_NO_THROW(Writer writer(
      makeFreshEmptyDir("ctor-allArgs"),
      IoMechanism::Stream,
      1024UL * 1024UL));
}

TEST(Writer, constructionRejectsMissingDirectory)
{
  const auto missing = fs::temp_directory_path() / "nioc-chronicleTest" / "doesNotExist";
  fs::remove_all(missing);
  EXPECT_THROW(Writer writer(missing), std::invalid_argument);
}

TEST(Writer, constructionRejectsFilePath)
{
  const auto parent = makeFreshEmptyDir("ctor-filePath");
  const auto filePath = parent / "notADirectory";
  { std::ofstream sink(filePath); }
  EXPECT_THROW(Writer writer(filePath), std::invalid_argument);
}

TEST(Writer, constructionRejectsNonEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("ctor-nonEmpty");
  { std::ofstream sink(dir / "leftover"); }
  EXPECT_THROW(Writer writer(dir), std::invalid_argument);
}

TEST(Writer, writeSpan)
{
  constexpr auto channelA = ChannelId{ 16983UL };
  constexpr auto channelB = ChannelId{ 68964786UL };
  const auto data = generateData();

  const auto logPath = [&]()
  {
    auto writer = Writer{ makeFreshEmptyDir("writeSpan") };

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
  constexpr auto channelA = ChannelId{ 16983UL };
  constexpr auto channelB = ChannelId{ 68964786UL };
  const auto data = generateData();

  auto spanCollection = std::vector<std::span<const std::byte>>{};
  spanCollection.reserve(10UL);
  for(size_t ii = 0UL; ii < 10UL; ++ii)
  {
    spanCollection.push_back(std::as_bytes(std::span(data)));
  }
  const auto totalSize = computeTotalSizeInBytes(spanCollection);

  const auto logPath = [&]()
  {
    auto writer = Writer{ makeFreshEmptyDir("writeCollectionOfSpan") };

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
  const auto dir = makeFreshEmptyDir("writerPath");
  auto writer = Writer{ dir };
  EXPECT_EQ(writer.path(), dir);
}

} // namespace nioc::chronicle
