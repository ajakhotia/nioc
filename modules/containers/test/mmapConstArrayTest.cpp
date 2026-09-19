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
namespace fs = std::filesystem;

// Read-only by construction, and a contiguous range.
static_assert(
    std::is_same_v<decltype(std::declval<const MmapConstArray<int>&>().data()), const int*>);
static_assert(
    std::is_same_v<decltype(std::declval<const MmapConstArray<int>&>().at(0)), const int&>);
static_assert(std::is_same_v<MmapConstArray<int>::const_iterator, std::span<const int>::iterator>);
static_assert(std::contiguous_iterator<MmapConstArray<int>::const_iterator>);
static_assert(std::ranges::contiguous_range<MmapConstArray<int>>);

// Writes 0, 1, 2, ... into a fresh array file at path.
void writeRamp(const fs::path& path, const std::size_t count)
{
  auto array = MmapArray<std::int32_t>{path, count};
  std::iota(array.begin(), array.end(), 0);
}

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class MmapConstArrayTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  MmapConstArrayTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(MmapConstArrayTest, readsAnExistingFile)
{
  constexpr auto kCount = std::size_t{6};
  const auto path = this->path() / "constArray";
  writeRamp(path, kCount);

  const auto array = MmapConstArray<std::int32_t>{path};
  ASSERT_EQ(array.size(), kCount);
  EXPECT_FALSE(array.empty());
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<std::int32_t>(index));
  }
}

TEST_F(MmapConstArrayTest, worksAsAContiguousRange)
{
  const auto path = this->path() / "constArrayRange";
  writeRamp(path, 5);

  const auto array = MmapConstArray<std::int32_t>{path};
  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0), 10);
}

TEST_F(MmapConstArrayTest, evictLeavesElementsReadable)
{
  constexpr auto kCount = std::size_t{4096};
  const auto path = this->path() / "constArrayEvict";
  writeRamp(path, kCount);

  const auto array = MmapConstArray<std::int32_t>{path};
  array.evict(array.begin(), array.end());

  EXPECT_EQ(std::accumulate(array.begin(), array.end(), 0LL), (kCount * (kCount - 1)) / 2);
}

TEST_F(MmapConstArrayTest, openingAMissingFileThrows)
{
  EXPECT_THROW((MmapConstArray<std::int32_t>{path() / "missingConst"}), std::runtime_error);
}

TEST_F(MmapConstArrayTest, openingAFileThatIsNotAWholeNumberOfElementsThrows)
{
  // Not a multiple of sizeof(int32_t), so the file cannot be a whole number of elements.
  const auto path = this->path() / "ragged";
  static_cast<void>(MmapArray<std::byte>{path, std::size_t{15}});

  EXPECT_THROW((MmapConstArray<std::int32_t>{path}), std::runtime_error);
}

TEST_F(MmapConstArrayTest, atReadsElementsAndThrowsOutOfRange)
{
  constexpr auto kCount = std::size_t{5};
  const auto path = this->path() / "constArrayAt";
  writeRamp(path, kCount);

  const auto array = MmapConstArray<std::int32_t>{path};
  for(auto index = std::size_t{0}; index < kCount; ++index)
  {
    EXPECT_EQ(array.at(index), static_cast<std::int32_t>(index));
  }

  EXPECT_NO_THROW(static_cast<void>(array.at(kCount - 1)));
  EXPECT_THROW(static_cast<void>(array.at(kCount)), std::out_of_range);
}

TEST_F(MmapConstArrayTest, moveTransfersOwnershipOfTheMapping)
{
  constexpr auto kCount = std::size_t{6};
  const auto path = this->path() / "movedConstArray";
  writeRamp(path, kCount);

  auto source = std::optional<MmapConstArray<std::int32_t>>{std::in_place, path};
  const auto* const data = source->data();

  const auto array = MmapConstArray<std::int32_t>{std::move(*source)};
  // Destroying the moved-from source must release nothing: the mapping now belongs to array.
  source.reset();

  EXPECT_EQ(array.data(), data);
  ASSERT_EQ(array.size(), kCount);
  EXPECT_EQ(array.at(5), 5);
}

} // namespace nioc::containers
