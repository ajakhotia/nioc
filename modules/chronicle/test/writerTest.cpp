////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/exception.hpp>
#include <numeric>
#include <thread>
#include <vector>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

namespace
{

std::vector<char> makeData()
{
  static constexpr auto kSize = 20UL;
  auto data = std::vector<char>(kSize);
  std::ranges::iota(data, kSize);
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

/// Verify that every file written under @p logRoot has its expected size: the
/// sequence file @p expectedSequenceSize, each channel's index file
/// @p expectedIndexSize, and each roll file @p expectedRollSize. Throws
/// std::logic_error on an unrecognised file.
void expectWrittenFileSizes(
    const fs::path& logRoot,
    const std::uintmax_t expectedSequenceSize,
    const std::uintmax_t expectedIndexSize,
    const std::uintmax_t expectedRollSize)
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
      EXPECT_EQ(fs::file_size(entity), expectedSequenceSize);
    }
    else if(entityPathString.ends_with(kIndexFileName))
    {
      EXPECT_EQ(fs::file_size(entity), expectedIndexSize);
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
  EXPECT_THROW(Writer{missing}, std::invalid_argument);
}

TEST(Writer, constructionRejectsFilePath)
{
  const auto parent = makeFreshEmptyDir("ctor-filePath");
  auto filePath = parent / "notADirectory";
  {
    const auto sink = std::ofstream(filePath);
  }
  EXPECT_THROW(Writer{filePath}, std::invalid_argument);
}

TEST(Writer, constructionRejectsNonEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("ctor-nonEmpty");
  {
    const auto sink = std::ofstream(dir / "leftover");
  }
  EXPECT_THROW(Writer{dir}, std::invalid_argument);
}

TEST(Writer, writeSpan)
{
  constexpr auto channelA = ChannelId{16983UL};
  constexpr auto channelB = ChannelId{68964786UL};
  const auto data = makeData();

  const auto logPath = [&]()
  {
    auto writer = Writer{makeFreshEmptyDir("writeSpan")};

    writer.write(channelA, std::as_bytes(std::span(data)));
    writer.write(channelB, std::as_bytes(std::span(data)));

    return writer.path();
  }();

  // Two writes (one per channel): two sequence entries in the log, one index entry per channel.
  expectWrittenFileSizes(logPath, 2 * sizeof(SequenceEntry), sizeof(IndexEntry), data.size());
}

TEST(Writer, writeCollectionOfSpan)
{
  constexpr auto channelA = ChannelId{16983UL};
  constexpr auto channelB = ChannelId{68964786UL};
  const auto data = makeData();

  constexpr auto kSpanCount = 10UL;
  auto spanCollection = std::vector(kSpanCount, std::as_bytes(std::span(data)));
  const auto totalSize = computeTotalSizeInBytes(spanCollection);

  const auto logPath = [&]()
  {
    auto writer = Writer{makeFreshEmptyDir("writeCollectionOfSpan")};

    writer.write(channelA, spanCollection);
    writer.write(channelB, spanCollection);

    return writer.path();
  }();

  // Two writes (one per channel): two sequence entries in the log, one index entry per channel.
  expectWrittenFileSizes(logPath, 2 * sizeof(SequenceEntry), sizeof(IndexEntry), totalSize);
}

TEST(Writer, path)
{
  const auto dir = makeFreshEmptyDir("writerPath");
  const auto writer = Writer{dir};
  EXPECT_EQ(writer.path(), dir);
}

TEST(Writer, concurrentWritersConserveEveryFrame)
{
  constexpr auto kThreads = 4UL;
  constexpr auto kFramesPerThread = 64UL;
  static constexpr auto kChannel = ChannelId{77UL};

  // Each frame's payload encodes (thread, index), so the read-back can prove conservation and
  // per-producer FIFO order even though the cross-thread interleaving is unspecified.
  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("concurrentWriters")};

    auto producers = std::vector<std::thread>{};
    producers.reserve(kThreads);
    for(auto threadId = 0UL; threadId < kThreads; ++threadId)
    {
      producers.emplace_back(
          [&writer, threadId]
          {
            for(auto index = 0UL; index < kFramesPerThread; ++index)
            {
              const auto payload = std::array{threadId, index};
              writer.write(kChannel, std::as_bytes(std::span(payload)));
            }
          });
    }
    for(auto& producer: producers)
    {
      producer.join();
    }

    return writer.path();
  }();

  auto reader = Reader{logPath};
  auto nextIndexPerThread = std::array<std::uint64_t, kThreads>{};
  for(auto count = 0UL; count < kThreads * kFramesPerThread; ++count)
  {
    const auto entry = reader.read();
    ASSERT_EQ(kChannel, entry.mChannelId);

    const auto span = entry.mMemoryCrate.span();
    ASSERT_EQ(2 * sizeof(std::uint64_t), span.size());
    auto payload = std::array<std::uint64_t, 2>{};
    std::memcpy(payload.data(), span.data(), span.size());

    const auto threadId = payload[0];
    ASSERT_LT(threadId, kThreads);
    // Per-producer FIFO: thread T's frames come back in the order T wrote them.
    EXPECT_EQ(nextIndexPerThread.at(threadId), payload[1]);
    ++nextIndexPerThread.at(threadId);
  }

  // Every frame is accounted for and nothing extra remains.
  for(const auto& produced: nextIndexPerThread)
  {
    EXPECT_EQ(kFramesPerThread, produced);
  }
  EXPECT_THROW(static_cast<void>(reader.read()), std::runtime_error);
}

TEST(Writer, mmapMechanismRejectedOnFirstWrite)
{
  constexpr auto channel = ChannelId{1UL};
  const auto data = makeData();

  // Construction accepts any mechanism; the unsupported one surfaces on the first frame, when the
  // channel writer would be built.
  auto writer = Writer{makeFreshEmptyDir("mmapRejected"), IoMechanism::Mmap};
  EXPECT_THROW(writer.write(channel, std::as_bytes(std::span(data))), std::invalid_argument);
}

} // namespace nioc::chronicle
