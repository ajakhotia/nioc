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
#include <nioc/common/filesystem.hpp>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace nioc::containers
{
namespace
{

// Compile-time guarantees the runtime tests cannot exercise: const-ness reaches the pointee (a
// const array hands out const access), and the array models a contiguous range.
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>().data()), const int*>);
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<MmapArray<int>&>().at(0)), int&>);
static_assert(std::is_same_v<decltype(std::declval<const MmapArray<int>&>().at(0)), const int&>);
static_assert(std::is_same_v<MmapArray<int>::iterator, std::span<int>::iterator>);
static_assert(std::is_same_v<MmapArray<int>::const_iterator, std::span<const int>::iterator>);
static_assert(std::is_same_v<
              decltype(std::declval<const MmapArray<int>&>().begin()),
              MmapArray<int>::const_iterator>);
static_assert(std::contiguous_iterator<MmapArray<int>::iterator>);
static_assert(std::ranges::contiguous_range<MmapArray<int>>);

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class MmapArrayTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  MmapArrayTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(MmapArrayTest, writesAreReadableThroughAConstArray)
{
  constexpr auto kCount = std::size_t{8};
  const auto path = this->path() / "array";

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
    EXPECT_EQ(array.at(index), static_cast<std::uint32_t>(index * index));
  }
}

TEST_F(MmapArrayTest, worksAsAContiguousRange)
{
  constexpr auto kCount = std::size_t{5};
  const auto path = this->path() / "arrayRange";

  auto array = MmapArray<int>{path, kCount};
  std::iota(array.begin(), array.end(), 1);

  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0), 15);
  EXPECT_EQ(std::span{array}.size(), kCount);
}

TEST_F(MmapArrayTest, evictLeavesElementsReadable)
{
  constexpr auto kCount = std::size_t{4096};
  const auto path = this->path() / "arrayEvict";

  auto array = MmapArray<int>{path, kCount};
  std::iota(array.begin(), array.end(), 0);

  array.evict(array.begin(), array.end());
  const auto& constArray = array;
  constArray.evict(constArray.cbegin(), constArray.cend());

  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0LL), (kCount * (kCount - 1)) / 2);
}

TEST_F(MmapArrayTest, resizeReducesTheElementCount)
{
  const auto path = this->path() / "arrayResize";

  {
    auto array = MmapArray<int>{path, 16};
    array.resize(4);
  }

  const auto array = MmapConstArray<int>{path};
  EXPECT_EQ(array.size(), 4U);
}

TEST_F(MmapArrayTest, atReturnsTheElementAndIsWritable)
{
  constexpr auto kCount = std::size_t{4};
  const auto path = this->path() / "arrayAt";

  auto array = MmapArray<int>{path, kCount};
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    array.at(index) = static_cast<int>(index * 16);
  }

  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<int>(index * 16));
    EXPECT_EQ(&array.at(index), &array[index]);
  }
}

TEST_F(MmapArrayTest, atThrowsWhenIndexIsOutOfRange)
{
  const auto path = this->path() / "arrayAtThrows";
  auto array = MmapArray<int>{path, 3};

  EXPECT_NO_THROW(static_cast<void>(array.at(2)));
  EXPECT_THROW(static_cast<void>(array.at(3)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(array.at(99)), std::out_of_range);
}

TEST_F(MmapArrayTest, moveTransfersOwnershipOfTheMapping)
{
  const auto path = this->path() / "movedArray";

  constexpr auto kMarker = std::int32_t{42}; // survives the move, proving the same bytes are read
  auto source = std::optional<MmapArray<std::int32_t>>{std::in_place, path, std::size_t{4}};
  source->at(0) = kMarker;
  const auto* const data = source->data();

  const auto array = MmapArray<std::int32_t>{std::move(*source)};
  // Destroying the moved-from source must release nothing: the mapping now belongs to array.
  source.reset();

  EXPECT_EQ(array.data(), data);
  EXPECT_EQ(array.at(0), kMarker);
}

} // namespace nioc::containers
