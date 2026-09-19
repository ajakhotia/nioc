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
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <nioc/containers/mmapRegion.hpp>
#include <numeric>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

namespace nioc::containers
{
namespace
{
namespace fs = std::filesystem;

fs::path freshPath(const std::string_view name)
{
  const auto directory = fs::temp_directory_path() / "nioc-containersTest";
  fs::create_directories(directory);
  const auto path = directory / name;
  fs::remove(path);
  return path;
}

/// @brief Whether each page of @p region is mapped into this process's page tables, read from
/// /proc/self/pagemap (one 64-bit entry per virtual page; bit 63 is the present bit).
std::vector<bool> mappedPages(const MmapRegion& region)
{
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  const auto pageCount = (region.size() + pageBytes - 1) / pageBytes;
  const auto firstPage = std::bit_cast<std::uintptr_t>(region.data()) / pageBytes;

  // std::ifstream cannot read this pseudo-file; the C stream API can.
  auto entries = std::vector<std::uint64_t>(pageCount);
  const auto pagemap = std::unique_ptr<std::FILE, decltype(&std::fclose)>{
      std::fopen("/proc/self/pagemap", "rbe"),
      &std::fclose};
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

} // namespace

TEST(MmapRegion, writesAreVisibleWhenReopenedReadOnly)
{
  const auto path = freshPath("region");

  {
    auto region = MmapRegion{path, 64};
    EXPECT_EQ(region.size(), 64);
    region.bytes().front() = std::byte{0xAB};
    region.bytes().back() = std::byte{0xCD};
  }

  EXPECT_EQ(fs::file_size(path), 64);

  const auto region = MmapRegion{path};
  EXPECT_EQ(region.size(), 64);
  EXPECT_EQ(region.bytes().front(), std::byte{0xAB});
  EXPECT_EQ(region.bytes().back(), std::byte{0xCD});
}

TEST(MmapRegion, iteratesAndViewsElements)
{
  constexpr auto kCount = std::size_t{16};
  const auto path = freshPath("regionElements");

  auto region = MmapRegion{path, kCount * sizeof(std::uint32_t)};
  std::ranges::fill(region, std::byte{0});
  auto words = region.elements<std::uint32_t>();
  ASSERT_EQ(words.size(), kCount);
  std::ranges::iota(words, 0U);

  const auto& constRegion = region;
  static_assert(std::is_same_v<decltype(constRegion.begin()), MmapRegion::const_iterator>);
  static_assert(std::is_same_v<decltype(region.begin()), MmapRegion::iterator>);
  EXPECT_EQ(std::distance(constRegion.begin(), constRegion.end()), kCount * sizeof(std::uint32_t));
  EXPECT_EQ(std::accumulate(words.begin(), words.end(), 0U), (kCount * (kCount - 1)) / 2);
  EXPECT_EQ(constRegion.elements<std::uint32_t>().back(), kCount - 1);
}

TEST(MmapRegion, resizeShrinksBackingFile)
{
  const auto path = freshPath("regionResize");

  {
    auto region = MmapRegion{path, 64};
    region.resize(16);
  }

  EXPECT_EQ(fs::file_size(path), 16);
}

TEST(MmapRegion, resizeExtendsBackingFile)
{
  const auto path = freshPath("regionExtend");

  {
    auto region = MmapRegion{path, 64};
    region.resize(256);
  }

  EXPECT_EQ(fs::file_size(path), 256);
}

TEST(MmapRegion, emptyFileMapsToEmptyRegion)
{
  const auto path = freshPath("emptyRegion");
  static_cast<void>(std::ofstream{path});
  ASSERT_EQ(fs::file_size(path), 0);

  const auto region = MmapRegion{path};
  EXPECT_TRUE(region.empty());
  EXPECT_EQ(region.size(), 0);
  EXPECT_TRUE(region.bytes().empty());
}

TEST(MmapRegion, openingAMissingFileThrows)
{
  EXPECT_THROW((MmapRegion{freshPath("missingRegion")}), std::runtime_error);
}

TEST(MmapRegion, moveTransfersOwnershipOfTheMapping)
{
  const auto path = freshPath("movedRegion");

  constexpr auto kMarker = std::byte{0xEF}; // survives the move, proving the same bytes are read
  auto source = std::optional<MmapRegion>{std::in_place, path, std::size_t{64}};
  source->bytes().front() = kMarker;
  const auto* const data = source->data();

  const auto region = MmapRegion{std::move(*source)};
  // Destroying the moved-from source must release nothing: the mapping now belongs to region.
  source.reset();

  EXPECT_EQ(region.data(), data);
  EXPECT_EQ(region.size(), 64);
  EXPECT_EQ(region.bytes().front(), kMarker);
}

TEST(MmapRegion, evictDropsOnlyThePagesInsideTheRange)
{
  constexpr auto kPageCount = std::size_t{8};
  constexpr auto kMarker = std::byte{0x5A};
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));

  const auto path = freshPath("evictedRegion");
  auto writable = MmapRegion{path, kPageCount * pageBytes};
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

TEST(MmapRegion, evictIgnoresRangesOutsideTheMapping)
{
  constexpr auto kRegionBytes = std::size_t{4096};
  constexpr auto kMarker = std::byte{0xA5};

  const auto path = freshPath("evictForeign");
  auto region = MmapRegion{path, kRegionBytes};
  std::ranges::fill(region.bytes(), kMarker);

  const auto foreign = std::array<std::byte, 16>{};
  region.evict(std::span<const std::byte>{foreign});
  region.evict(std::span<const std::byte>{});

  const auto emptyPath = freshPath("evictEmpty");
  static_cast<void>(std::ofstream{emptyPath});
  const auto empty = MmapRegion{emptyPath};
  empty.evict(empty.bytes());

  EXPECT_TRUE(std::ranges::all_of(region.bytes(), [](const auto byte) { return byte == kMarker; }));
}

} // namespace nioc::containers
