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
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace nioc::containers
{
namespace
{
namespace fs = std::filesystem;

// Compile-time guarantees the runtime tests cannot exercise: const-ness reaches the pointee (a
// const array hands out const access), and the array models a contiguous range with raw-pointer
// iterators.
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>().data()), const int*>);
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>().at(0)), int&>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>().at(0)), const int&>);
static_assert(std::is_same_v<MmapArray<int>::iterator, int*>);
static_assert(std::contiguous_iterator<MmapArray<int>::iterator>);
static_assert(std::ranges::contiguous_range<MmapArray<int>>);

fs::path freshPath(const std::string_view name)
{
  const auto directory = fs::temp_directory_path() / "nioc-containersTest";
  fs::create_directories(directory);
  const auto path = directory / name;
  fs::remove(path);
  return path;
}

} // namespace

TEST(MmapArray, writesAreReadableThroughAConstArray)
{
  constexpr auto kCount = std::size_t{8};
  const auto path = freshPath("array");

  {
    auto array = MmapArray<std::uint32_t>{path, kCount};
    EXPECT_EQ(array.size(), kCount);
    EXPECT_FALSE(array.empty());
    for(auto index = std::size_t{0}; index < kCount; ++index)
    {
      array[index] = static_cast<std::uint32_t>(index * index);
    }
  }

  const auto array = MmapConstArray<std::uint32_t>{path};
  ASSERT_EQ(array.size(), kCount);
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array[index], static_cast<std::uint32_t>(index * index));
  }
}

TEST(MmapArray, worksAsAContiguousRange)
{
  constexpr auto kCount = std::size_t{5};
  const auto path = freshPath("arrayRange");

  auto array = MmapArray<int>{path, kCount};
  std::iota(array.begin(), array.end(), 1);

  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0), 15);
  EXPECT_EQ(std::span{array}.size(), kCount);
}

TEST(MmapArray, resizeReducesTheElementCount)
{
  const auto path = freshPath("arrayResize");

  {
    auto array = MmapArray<int>{path, 10};
    array.resize(4);
  }

  const auto array = MmapConstArray<int>{path};
  EXPECT_EQ(array.size(), 4U);
}

TEST(MmapArray, atReturnsTheElementAndIsWritable)
{
  constexpr auto kCount = std::size_t{4};
  const auto path = freshPath("arrayAt");

  auto array = MmapArray<int>{path, kCount};
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    array.at(index) = static_cast<int>(index * 10);
  }

  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<int>(index * 10));
    EXPECT_EQ(&array.at(index), &array[index]);
  }
}

TEST(MmapArray, atThrowsWhenIndexIsOutOfRange)
{
  const auto path = freshPath("arrayAtThrows");
  auto array = MmapArray<int>{path, 3};

  EXPECT_NO_THROW(static_cast<void>(array.at(2)));
  EXPECT_THROW(static_cast<void>(array.at(3)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(array.at(99)), std::out_of_range);
}

} // namespace nioc::containers
