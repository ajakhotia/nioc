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

constexpr auto channelA = ChannelId{ 16983UL };
constexpr auto channelB = ChannelId{ 68964786UL };
const auto kDataA = generateData(20ULL);
const auto kDataB = generateData(34ULL);
const auto kDataAAsBytes = std::as_bytes(std::span(kDataA));
const auto kDataBAsBytes = std::as_bytes(std::span(kDataB));

fs::path createLog()
{
  auto writer = Writer{ makeFreshEmptyDir("readerTest-createLog") };

  writer.write(channelA, kDataAAsBytes);
  writer.write(channelB, kDataBAsBytes);
  writer.write(channelA, kDataAAsBytes);
  writer.write(channelB, kDataBAsBytes);

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
  auto reader = Reader{ logPath };

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelA, entry.mChannelId);
    expectSpanEqual(kDataAAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelB, entry.mChannelId);
    expectSpanEqual(kDataBAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelA, entry.mChannelId);
    expectSpanEqual(kDataAAsBytes, entry.mMemoryCrate.span());
  }

  {
    const auto entry = reader.read();
    EXPECT_EQ(channelB, entry.mChannelId);
    expectSpanEqual(kDataBAsBytes, entry.mMemoryCrate.span());
  }

  EXPECT_THROW(reader.read(), std::runtime_error);
}


} // namespace nioc::chronicle
