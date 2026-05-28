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
#include <nioc/common/exception.hpp>
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

/// Verify that every file written under @p logRoot has the size expected for
/// its type. Sequence and index files have fixed sizes; each roll file must
/// equal the @p expectedRollSize. Throws std::logic_error on an unrecognised file.
void expectWrittenFileSizes(const fs::path& logRoot, const std::uintmax_t expectedRollSize)
{
  for(const auto& entity: fs::recursive_directory_iterator(logRoot))
  {
    if(fs::is_directory(entity))
    {
      continue;
    }

    if(const auto& entityPathString = entity.path().string();
       entityPathString.ends_with(kSequenceFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 16);
    }
    else if(entityPathString.ends_with(kIndexFileName))
    {
      EXPECT_EQ(fs::file_size(entity), 24);
    }
    else if(entityPathString.ends_with(kRollFileNameExtension))
    {
      EXPECT_EQ(fs::file_size(entity), expectedRollSize);
    }
    else
    {
      common::throwException<std::logic_error>(
          "Unexpected file type encountered: {}",
          entityPathString);
    }
  }
}

} // namespace

TEST(Writer, constructionAcceptsEmptyDirectory)
{
  EXPECT_NO_THROW(Writer(makeFreshEmptyDir("ctor-default")));
  EXPECT_NO_THROW(Writer(makeFreshEmptyDir("ctor-allArgs"), IoMechanism::Stream, 1024UL * 1024UL));
}

TEST(Writer, constructionRejectsMissingDirectory)
{
  const auto missing = fs::temp_directory_path() / "nioc-chronicleTest" / "doesNotExist";
  fs::remove_all(missing);
  EXPECT_THROW(Writer{ missing }, std::invalid_argument);
}

TEST(Writer, constructionRejectsFilePath)
{
  const auto parent = makeFreshEmptyDir("ctor-filePath");
  auto filePath = parent / "notADirectory";
  {
    const auto sink = std::ofstream(filePath);
  }
  EXPECT_THROW(Writer{ filePath }, std::invalid_argument);
}

TEST(Writer, constructionRejectsNonEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("ctor-nonEmpty");
  {
    const auto sink = std::ofstream(dir / "leftover");
  }
  EXPECT_THROW(Writer{ dir }, std::invalid_argument);
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

  expectWrittenFileSizes(logPath, data.size());
}

TEST(Writer, writeCollectionOfSpan)
{
  constexpr auto channelA = ChannelId{ 16983UL };
  constexpr auto channelB = ChannelId{ 68964786UL };
  const auto data = generateData();

  constexpr auto kSpanCount = 10UL;
  auto spanCollection = std::vector(kSpanCount, std::as_bytes(std::span(data)));
  const auto totalSize = computeTotalSizeInBytes(spanCollection);

  const auto logPath = [&]()
  {
    auto writer = Writer{ makeFreshEmptyDir("writeCollectionOfSpan") };

    writer.write(channelA, spanCollection);
    writer.write(channelB, spanCollection);

    return writer.path();
  }();

  expectWrittenFileSizes(logPath, totalSize);
}

TEST(Writer, path)
{
  const auto dir = makeFreshEmptyDir("writerPath");
  const auto writer = Writer{ dir };
  EXPECT_EQ(writer.path(), dir);
}

} // namespace nioc::chronicle
