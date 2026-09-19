////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <nioc/common/filesystem.hpp>
#include <nioc/containers/file.hpp>
#include <nioc/containers/mapping.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

namespace nioc::containers
{
namespace
{

/// @brief Whether each page of @p region is mapped into this process's page tables, read from
/// /proc/self/pagemap (one 64-bit entry per virtual page; bit 63 is the present bit).
std::vector<bool> mappedPages(const Mapping& mapping)
{
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  const auto pageCount = (mapping.size() + pageBytes - 1) / pageBytes;
  const auto firstPage = std::bit_cast<std::uintptr_t>(mapping.data()) / pageBytes;

  // std::ifstream cannot read this pseudo-file; the C stream API can.
  auto entries = std::vector<std::uint64_t>(pageCount);

  constexpr auto closeFile = [](std::FILE* file)
  {
    static_cast<void>(std::fclose(file)); // NOLINT(cppcoreguidelines-owning-memory)
  };
  const auto pagemap = std::unique_ptr<std::FILE, decltype(closeFile)>{
      std::fopen("/proc/self/pagemap", "rbe")};
  if(pagemap == nullptr or
     ::fseeko(pagemap.get(), static_cast<off_t>(firstPage * sizeof(std::uint64_t)), SEEK_SET) !=
         0 or
     std::fread(entries.data(), sizeof(std::uint64_t), pageCount, pagemap.get()) != pageCount)
  {
    ADD_FAILURE() << "Cannot read /proc/self/pagemap";
    return {};
  }

  constexpr auto kPresentBit = std::uint64_t{1} << 63U;
  auto mapped = std::vector<bool>{};
  std::ranges::transform(
      entries,
      std::back_inserter(mapped),
      [](const auto entry) { return (entry & kPresentBit) != 0U; });
  return mapped;
}

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class MappingTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  MappingTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(MappingTest, readWriteReachesTheFileAndAReadOnlyMapping)
{
  constexpr auto kBytes = std::size_t{4096};
  constexpr auto kMarker = std::byte{0x3C};
  const auto file = File::create(path() / "mappingShared", kBytes);

  auto writable = Mapping::readWrite(file, kBytes);
  const auto readable = Mapping::readOnly(file, kBytes);
  ASSERT_EQ(writable.size(), kBytes);
  ASSERT_EQ(readable.size(), kBytes);

  std::ranges::fill(writable, kMarker);
  EXPECT_TRUE(std::ranges::all_of(readable, [](const auto byte) { return byte == kMarker; }));
}

TEST_F(MappingTest, offsetMapsATailOfTheFile)
{
  constexpr auto kMarker = std::byte{0x7E};
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  const auto file = File::create(path() / "mappingOffset", 3 * pageBytes);
  auto whole = Mapping::readWrite(file, 3 * pageBytes);
  whole.bytes().back() = kMarker;

  const auto tail = Mapping::readOnly(file, pageBytes, static_cast<std::int64_t>(2 * pageBytes));
  ASSERT_EQ(tail.size(), pageBytes);
  EXPECT_EQ(tail.bytes().back(), kMarker);
}

TEST_F(MappingTest, iteratesAsAByteRange)
{
  constexpr auto kBytes = std::size_t{64};
  const auto file = File::create(path() / "mappingIterate", kBytes);
  auto mapping = Mapping::readWrite(file, kBytes);

  static_assert(std::is_same_v<decltype(mapping.begin()), Mapping::iterator>);
  static_assert(std::is_same_v<decltype(std::as_const(mapping).begin()), Mapping::const_iterator>);
  std::ranges::fill(mapping, std::byte{1});
  EXPECT_EQ(std::distance(mapping.cbegin(), mapping.cend()), kBytes);
  EXPECT_TRUE(std::ranges::all_of(mapping, [](const auto byte) { return byte == std::byte{1}; }));
}

TEST_F(MappingTest, zeroLengthIsEmpty)
{
  const auto file = File::create(path() / "mappingEmpty", 0);
  const auto mapping = Mapping::readOnly(file, 0);
  EXPECT_TRUE(mapping.empty());
  EXPECT_EQ(mapping.size(), 0);
  EXPECT_TRUE(mapping.bytes().empty());
}

TEST_F(MappingTest, mappingBeyondTheFileThrows)
{
  const auto file = File::create(path() / "mappingBeyond", 16);
  const auto pageBytes = static_cast<std::int64_t>(::sysconf(_SC_PAGESIZE));
  // An offset past the end is accepted by mmap; an unaligned one is not, which is what is tested.
  EXPECT_THROW(static_cast<void>(Mapping::readOnly(file, 16, pageBytes + 1)), std::runtime_error);
}

TEST_F(MappingTest, moveTransfersOwnership)
{
  constexpr auto kBytes = std::size_t{4096};
  const auto file = File::create(path() / "mappingMove", kBytes);
  auto source = Mapping::readWrite(file, kBytes);
  const auto* const data = source.data();

  const auto moved = Mapping{std::move(source)};
  EXPECT_EQ(moved.data(), data);
  EXPECT_EQ(moved.size(), kBytes);
}

TEST_F(MappingTest, evictDropsOnlyThePagesInsideTheRange)
{
  constexpr auto kPageCount = std::size_t{8};
  constexpr auto kMarker = std::byte{0x5A};
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));

  const auto file = File::create(path() / "evictedMapping", kPageCount * pageBytes);
  auto writable = Mapping::readWrite(file, file.size());
  std::ranges::fill(writable.bytes(), kMarker);
  const auto& region = writable;
  ASSERT_EQ(mappedPages(region), std::vector<bool>(kPageCount, true));

  // A quarter page into page 1 through a quarter page into page 5: pages 2 to 4 lie wholly
  // inside; the two partly covered edge pages stay mapped.
  region.evict(region.bytes().subspan(pageBytes + (pageBytes / 4), 4 * pageBytes));
  EXPECT_EQ(
      mappedPages(region),
      (std::vector<bool>{true, true, false, false, false, true, true, true}));

  // Narrower than a page: nothing is evicted.
  region.evict(region.bytes().subspan(0, pageBytes - 1));
  EXPECT_TRUE(mappedPages(region).front());

  // The whole mapping evicts every page, and re-reading finds the content intact.
  region.evict(region.bytes());
  EXPECT_EQ(mappedPages(region), std::vector<bool>(kPageCount, false));
  EXPECT_TRUE(std::ranges::all_of(region.bytes(), [](const auto byte) { return byte == kMarker; }));
}

TEST_F(MappingTest, evictIgnoresRangesOutsideTheMapping)
{
  constexpr auto kRegionBytes = std::size_t{4096};
  constexpr auto kMarker = std::byte{0xA5};

  const auto file = File::create(path() / "evictForeign", kRegionBytes);
  auto region = Mapping::readWrite(file, kRegionBytes);
  std::ranges::fill(region.bytes(), kMarker);

  const auto foreign = std::array<std::byte, 16>{};
  region.evict(std::span<const std::byte>{foreign});
  region.evict(std::span<const std::byte>{});

  const auto emptyFile = File::create(path() / "evictEmpty", 0);
  const auto empty = Mapping::readOnly(emptyFile, 0);
  empty.evict(empty.bytes());

  EXPECT_TRUE(std::ranges::all_of(region.bytes(), [](const auto byte) { return byte == kMarker; }));
}

} // namespace nioc::containers
