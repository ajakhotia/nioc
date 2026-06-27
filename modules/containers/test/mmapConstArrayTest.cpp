////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <iterator>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace nioc::containers
{
namespace
{
namespace fs = std::filesystem;

// Read-only by construction, and a contiguous range with raw-pointer iterators.
static_assert(
    std::is_same_v<decltype(std::declval<const MmapConstArray<int>&>().data()), const int*>);
static_assert(
    std::is_same_v<decltype(std::declval<const MmapConstArray<int>&>().at(0)), const int&>);
static_assert(std::is_same_v<MmapConstArray<int>::const_iterator, const int*>);
static_assert(std::contiguous_iterator<MmapConstArray<int>::const_iterator>);
static_assert(std::ranges::contiguous_range<MmapConstArray<int>>);

fs::path freshPath(const std::string_view name)
{
  const auto directory = fs::temp_directory_path() / "nioc-containersTest";
  fs::create_directories(directory);
  const auto path = directory / name;
  fs::remove(path);
  return path;
}

// Writes 0, 1, 2, ... into a fresh array file and returns its path.
fs::path writeRamp(const std::string_view name, const std::size_t count)
{
  const auto path = freshPath(name);
  auto array = MmapArray<std::int32_t>{path, count};
  std::iota(array.begin(), array.end(), 0);
  return path;
}

} // namespace

TEST(MmapConstArray, readsAnExistingFile)
{
  constexpr auto kCount = std::size_t{6};
  const auto path = writeRamp("constArray", kCount);

  const auto array = MmapConstArray<std::int32_t>{path};
  ASSERT_EQ(array.size(), kCount);
  EXPECT_FALSE(array.empty());
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<std::int32_t>(index));
  }
}

TEST(MmapConstArray, worksAsAContiguousRange)
{
  const auto path = writeRamp("constArrayRange", 5);

  const auto array = MmapConstArray<std::int32_t>{path};
  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0), 10);
}

TEST(MmapConstArray, openingAMissingFileThrows)
{
  EXPECT_THROW((MmapConstArray<std::int32_t>{freshPath("missingConst")}), std::runtime_error);
}

TEST(MmapConstArray, openingAFileThatIsNotAWholeNumberOfElementsThrows)
{
  // Not a multiple of sizeof(int32_t), so the file cannot be a whole number of elements.
  const auto path = freshPath("ragged");
  static_cast<void>(MmapArray<std::byte>{path, std::size_t{15}});

  EXPECT_THROW((MmapConstArray<std::int32_t>{path}), std::runtime_error);
}

TEST(MmapConstArray, atReadsElementsAndThrowsOutOfRange)
{
  constexpr auto kCount = std::size_t{5};
  const auto path = writeRamp("constArrayAt", kCount);

  const auto array = MmapConstArray<std::int32_t>{path};
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<std::int32_t>(index));
  }

  EXPECT_NO_THROW(static_cast<void>(array.at(kCount - 1)));
  EXPECT_THROW(static_cast<void>(array.at(kCount)), std::out_of_range);
}

} // namespace nioc::containers
