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

TEST(MmapRegion, resizeShrinksBackingFile)
{
  const auto path = freshPath("regionResize");

  {
    auto region = MmapRegion{path, 64};
    region.resize(16);
  }

  EXPECT_EQ(fs::file_size(path), 16);
}

TEST(MmapRegion, openingAMissingFileThrows)
{
  EXPECT_THROW((MmapRegion{freshPath("missingRegion")}), std::runtime_error);
}

} // namespace nioc::containers
