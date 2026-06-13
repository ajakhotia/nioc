////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <gtest/gtest.h>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <numeric>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

namespace
{
std::vector<char> generateData(std::uint64_t size)
{
  auto data = std::vector<char>(size);
  std::ranges::iota(data, size);
  return data;
}

/// Create a fresh empty directory at a deterministic path under the system temp
/// directory. Any prior contents are wiped.
fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

constexpr auto channelA = ChannelId{16983UL};
constexpr auto channelB = ChannelId{68964786UL};
constexpr auto dataASize = 20ULL;
constexpr auto dataBSize = 34ULL;

std::vector<char> dataA()
{
  return generateData(dataASize);
}

std::vector<char> dataB()
{
  return generateData(dataBSize);
}

fs::path createLog()
{
  const auto dataAValue = dataA();
  const auto dataBValue = dataB();
  const auto dataAAsBytes = std::as_bytes(std::span(dataAValue));
  const auto dataBAsBytes = std::as_bytes(std::span(dataBValue));

  auto writer = Writer{makeFreshEmptyDir("readerTest-createLog")};

  writer.write(channelA, dataAAsBytes);
  writer.write(channelB, dataBAsBytes);
  writer.write(channelA, dataAAsBytes);
  writer.write(channelB, dataBAsBytes);

  return writer.path();
}

void expectSpanEqual(const std::span<const std::byte>& lhs, const std::span<const std::byte>& rhs)
{
  EXPECT_TRUE(std::ranges::equal(lhs, rhs));
}

} // namespace

TEST(Reader, read)
{
  const auto logPath = createLog();
  auto reader = Reader{logPath};

  const auto dataAValue = dataA();
  const auto dataBValue = dataB();
  const auto dataAAsBytes = std::as_bytes(std::span(dataAValue));
  const auto dataBAsBytes = std::as_bytes(std::span(dataBValue));

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelA, entry.mChannelId);
    expectSpanEqual(dataAAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelB, entry.mChannelId);
    expectSpanEqual(dataBAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelA, entry.mChannelId);
    expectSpanEqual(dataAAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelB, entry.mChannelId);
    expectSpanEqual(dataBAsBytes, entry.mMemoryCrate.span());
  }

  EXPECT_THROW(reader.read(), std::runtime_error);
}

TEST(Reader, constructionRejectsMissingDirectory)
{
  const auto missing = fs::temp_directory_path() / "nioc-chronicleTest" / "absent";
  fs::remove_all(missing);
  EXPECT_THROW(Reader{missing}, std::invalid_argument);
}

// KNOWN HOLE: IoMechanism::Stream has a writer but no reader, so a chronicle recorded with the
// default Stream mechanism can only be replayed through Mmap. Conceptually each mechanism that
// writes should also read back; this test states that expectation and fails until a stream
// channel reader exists.
// TEST(Reader, streamMechanismReadsBackWrittenData)
//{
//  const auto logPath = createLog();
//  auto reader = Reader{logPath, IoMechanism::Stream};
//
//  const auto dataAValue = dataA();
//  const auto entry = reader.read();
//  EXPECT_EQ(channelA, entry.mChannelId);
//  expectSpanEqual(std::as_bytes(std::span(dataAValue)), entry.mMemoryCrate.span());
//}


} // namespace nioc::chronicle
