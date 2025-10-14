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
  std::iota(data.begin(), data.end(), size);
  return data;
}

constexpr auto channelA = ChannelId{ 16983UL };
constexpr auto channelB = ChannelId{ 68964786UL };
const auto dataA = generateData(20ULL);
const auto dataB = generateData(34ULL);
const auto dataAAsBytes = std::as_bytes(std::span(dataA));
const auto dataBAsBytes = std::as_bytes(std::span(dataB));

fs::path createLog()
{
  auto writer = Writer{};

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
  auto reader = Reader{ logPath };

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


} // namespace nioc::chronicle
