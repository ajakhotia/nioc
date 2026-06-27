////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstddef>
#include <functional>
#include <gtest/gtest.h>
#include <iterator>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

static_assert(std::input_iterator<Reader::Iterator>);
static_assert(std::ranges::input_range<Reader>);

namespace
{

constexpr auto channelA = ChannelId{16983ULL};
constexpr auto channelB = ChannelId{68964786ULL};

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

fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

void expectBytesEqual(std::span<const std::byte> lhs, std::span<const std::byte> rhs)
{
  EXPECT_TRUE(std::ranges::equal(lhs, rhs));
}

} // namespace

TEST(Reader, readsFramesInRecordOrder)
{
  const auto dataA = makeBytes(20, std::byte{1});
  const auto dataB = makeBytes(34, std::byte{100});

  const auto logPath = [&]
  {
    auto writer = Writer{makeFreshEmptyDir("reader-recordOrder"), 256};
    writer.write(channelA, dataA);
    writer.write(channelB, dataB);
    writer.write(channelA, dataA);
    writer.write(channelB, dataB);
    return writer.path();
  }();

  const auto expected = std::array{
      std::pair{channelA, std::cref(dataA)},
      std::pair{channelB, std::cref(dataB)},
      std::pair{channelA, std::cref(dataA)},
      std::pair{channelB, std::cref(dataB)}};

  auto index = std::size_t{0};
  for(const auto& entry: Reader{logPath})
  {
    ASSERT_LT(index, expected.size());
    EXPECT_EQ(expected.at(index).first, entry.mChannelId);
    expectBytesEqual(expected.at(index).second.get(), entry.mCrate.span());
    ++index;
  }
  EXPECT_EQ(index, expected.size());
}

TEST(Reader, emptyChronicleHasNoEntries)
{
  const auto logPath = [&]
  {
    const auto writer = Writer{makeFreshEmptyDir("reader-empty")};
    return writer.path();
  }();

  auto reader = Reader{logPath};
  EXPECT_TRUE(reader.begin() == reader.end());
}

TEST(Reader, constructionRejectsMissingDirectory)
{
  const auto missing = fs::temp_directory_path() / "nioc-chronicleTest" / "absent";
  fs::remove_all(missing);
  EXPECT_THROW(Reader{missing}, std::invalid_argument);
}

} // namespace nioc::chronicle
