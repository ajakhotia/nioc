////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/containers/mmapRegion.hpp>
#include <stdexcept>
#include <string_view>

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

} // namespace

TEST(MmapRegion, writesAreVisibleWhenReopenedReadOnly)
{
  constexpr auto kByteSize = std::size_t{64};
  const auto path = freshPath("region");

  {
    auto region = MmapRegion{path, kByteSize};
    EXPECT_EQ(region.size(), kByteSize);
    region.data()[0] = std::byte{0xAB};
    region.data()[kByteSize - 1] = std::byte{0xCD};
  }

  EXPECT_EQ(fs::file_size(path), kByteSize);

  const auto region = MmapRegion{path};
  EXPECT_EQ(region.size(), kByteSize);
  EXPECT_EQ(region.data()[0], std::byte{0xAB});
  EXPECT_EQ(region.data()[kByteSize - 1], std::byte{0xCD});
}

TEST(MmapRegion, resizeShrinksBackingFile)
{
  const auto path = freshPath("regionResize");

  {
    auto region = MmapRegion{path, 100};
    region.resize(40);
  }

  EXPECT_EQ(fs::file_size(path), 40U);
}

TEST(MmapRegion, openingAMissingFileThrows)
{
  EXPECT_THROW((MmapRegion{freshPath("missingRegion")}), std::runtime_error);
}

} // namespace nioc::containers
