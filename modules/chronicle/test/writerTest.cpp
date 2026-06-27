////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/chronicle/channel.hpp>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <span>
#include <thread>
#include <utility>
#include <vector>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

namespace
{

constexpr auto channelA = ChannelId{16983ULL};
constexpr auto channelB = ChannelId{68964786ULL};

// Test helper: size and start byte are intentionally separate scalar parameters.
std::vector<std::byte> makeBytes(const std::size_t size, const std::byte start = std::byte{0})
{
  auto bytes = std::vector<std::byte>(size);
  for(auto index = std::size_t{0}; index < size; ++index)
  {
    bytes.at(index) = std::byte(
        static_cast<unsigned char>(std::to_integer<unsigned char>(start) + index));
  }
  return bytes;
}

void fill(const Reservation& reservation, const std::span<const std::byte> source)
{
  std::memcpy(reservation.span().data(), source.data(), source.size());
}

fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

fs::path rollPath(const fs::path& logRoot, const ChannelId channelId, const std::uint64_t rollId)
{
  return logRoot / hexString(channelId.mValue) / buildRollName(rollId);
}

void expectBytesEqual(std::span<const std::byte> lhs, std::span<const std::byte> rhs)
{
  EXPECT_TRUE(std::ranges::equal(lhs, rhs));
}

std::vector<Entry> drain(const fs::path& logRoot)
{
  auto entries = std::vector<Entry>{};
  for(const auto& entry: Reader{logRoot})
  {
    entries.push_back(entry);
  }
  return entries;
}

} // namespace

TEST(Writer, constructionAcceptsEmptyDirectory)
{
  EXPECT_NO_THROW(Writer(makeFreshEmptyDir("ctor-default")));
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
  const auto filePath = parent / "notADirectory";
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

TEST(Writer, writeOneShotRoundTrips)
{
  const auto dataA = makeBytes(20, std::byte{1});
  const auto dataB = makeBytes(34, std::byte{100});

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("writeOneShot"), 256};
    writer.write(channelA, dataA);
    writer.write(channelB, dataB);
    return writer.path();
  }();

  const auto entries = drain(logPath);

  ASSERT_EQ(2U, entries.size());
  EXPECT_EQ(channelA, entries.at(0).mChannelId);
  expectBytesEqual(dataA, entries.at(0).mCrate.span());
  EXPECT_EQ(channelB, entries.at(1).mChannelId);
  expectBytesEqual(dataB, entries.at(1).mCrate.span());
}

TEST(Writer, reserveAndRecordBuildsInPlace)
{
  const auto first = makeBytes(20, std::byte{1});
  const auto second = makeBytes(30, std::byte{50});

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("reserveAndRecord"), 256};
    auto& channel = writer.channel(channelA);

    {
      auto reservation = channel.reserve(64);
      fill(reservation, first);
      static_cast<void>(std::move(reservation).commit(first.size()));
    } // making the crate records the frame's timeline entry

    {
      auto reservation = channel.reserve(64);
      fill(reservation, second);
      static_cast<void>(std::move(reservation).commit(second.size()));
    }

    return writer.path();
  }();

  const auto entries = drain(logPath);
  ASSERT_EQ(2U, entries.size());
  expectBytesEqual(first, entries.at(0).mCrate.span());
  expectBytesEqual(second, entries.at(1).mCrate.span());
}

TEST(Writer, rollsOverWhenFull)
{
  const auto frame = makeBytes(100, std::byte{7});

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("rollsOver"), 128};
    auto& channel = writer.channel(channelA);
    channel.write(frame); // roll 0
    channel.write(frame); // would overflow roll 0 (104 + 104 > 128) -> roll 1
    return writer.path();
  }();

  EXPECT_TRUE(fs::exists(rollPath(logPath, channelA, 0)));
  EXPECT_TRUE(fs::exists(rollPath(logPath, channelA, 1)));

  const auto entries = drain(logPath);
  ASSERT_EQ(2U, entries.size());
  expectBytesEqual(frame, entries.at(0).mCrate.span());
  expectBytesEqual(frame, entries.at(1).mCrate.span());
}

TEST(Writer, framesLargerThanRollCapacityGetOwnRoll)
{
  const auto big = makeBytes(300, std::byte{3});

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("bigFrame"), 128};
    writer.write(channelA, big);
    return writer.path();
  }();

  const auto entries = drain(logPath);
  ASSERT_EQ(1U, entries.size());
  expectBytesEqual(big, entries.at(0).mCrate.span());
}

TEST(Writer, concurrentChannelsConserveEveryFrame)
{
  constexpr auto kThreads = 4ULL;
  constexpr auto kFramesPerThread = 64ULL;

  // Each thread owns its own channel (single producer per channel) and records (thread, index)
  // pairs so the read-back can prove conservation and per-channel FIFO order.
  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("concurrentChannels")};

    auto producers = std::vector<std::thread>{};
    producers.reserve(kThreads);
    for(auto threadId = 0ULL; threadId < kThreads; ++threadId)
    {
      producers.emplace_back(
          [&writer, threadId]
          {
            auto& channel = writer.channel(ChannelId{threadId});
            for(auto index = 0ULL; index < kFramesPerThread; ++index)
            {
              const auto payload = std::array{threadId, index};
              channel.write(std::as_bytes(std::span(payload)));
            }
          });
    }
    for(auto& producer: producers)
    {
      producer.join();
    }

    return writer.path();
  }();

  auto nextIndexPerThread = std::array<std::uint64_t, kThreads>{};
  auto count = 0ULL;
  for(const auto& entry: Reader{logPath})
  {
    const auto span = entry.mCrate.span();
    ASSERT_EQ(2 * sizeof(std::uint64_t), span.size());
    auto payload = std::array<std::uint64_t, 2>{};
    std::memcpy(payload.data(), span.data(), span.size());

    const auto threadId = payload.at(0);
    ASSERT_LT(threadId, kThreads);
    EXPECT_EQ(ChannelId{threadId}, entry.mChannelId);
    EXPECT_EQ(nextIndexPerThread.at(threadId), payload.at(1));
    ++nextIndexPerThread.at(threadId);
    ++count;
  }
  EXPECT_EQ(kThreads * kFramesPerThread, count);

  for(const auto& produced: nextIndexPerThread)
  {
    EXPECT_EQ(kFramesPerThread, produced);
  }
}

TEST(Writer, recordsAcrossChannelsAndReplaysInGlobalOrder)
{
  constexpr auto kFrameCount = 5U;

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("timelineOrder"), 256};
    for(auto index = std::uint8_t{0}; index < kFrameCount; ++index)
    {
      const auto frame = makeBytes(4, std::byte{index});
      // Alternate channels so the single timeline is what carries the global order.
      writer.write(index % 2 == 0 ? channelA : channelB, frame);
    }
    return writer.path();
  }();

  // The timeline is a single file at the chronicle root carrying the whole global order.
  EXPECT_TRUE(fs::exists(logPath / kTimelineFileName));

  auto index = std::uint8_t{0};
  for(const auto& entry: Reader{logPath})
  {
    EXPECT_EQ(index % 2 == 0 ? channelA : channelB, entry.mChannelId);
    expectBytesEqual(makeBytes(4, std::byte{index}), entry.mCrate.span());
    ++index;
  }
  EXPECT_EQ(kFrameCount, index);
}

TEST(Writer, path)
{
  const auto dir = makeFreshEmptyDir("writerPath");
  const auto writer = Writer{dir};
  EXPECT_EQ(writer.path(), dir);
}

} // namespace nioc::chronicle
