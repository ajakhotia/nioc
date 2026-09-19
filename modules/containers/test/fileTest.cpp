////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/containers/file.hpp>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include <utility>

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

/// True when @p descriptor is still open in this process.
bool isOpen(const int descriptor)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): fcntl is the POSIX file API.
  return ::fcntl(descriptor, F_GETFD) != -1;
}

} // namespace

TEST(File, createSizesAndReportsThePath)
{
  constexpr auto kBytes = std::size_t{1234};
  const auto path = freshPath("nested/fileCreate");
  const auto file = File::create(path, kBytes);

  EXPECT_EQ(file.path(), path);
  EXPECT_GE(file.nativeHandle(), 0);
  EXPECT_EQ(file.size(), kBytes);
  EXPECT_EQ(fs::file_size(path), kBytes);
}

TEST(File, createTruncatesAnExistingFile)
{
  const auto path = freshPath("fileTruncate");
  static_cast<void>(File::create(path, 64));
  const auto file = File::create(path, 8);
  EXPECT_EQ(file.size(), 8U);
}

TEST(File, openReadsAnExistingFile)
{
  constexpr auto kBytes = std::size_t{32};
  const auto path = freshPath("fileOpen");
  static_cast<void>(File::create(path, kBytes));
  const auto file = File::open(path);
  EXPECT_EQ(file.size(), kBytes);
  EXPECT_EQ(file.path(), path);
}

TEST(File, openingAMissingFileThrows)
{
  EXPECT_THROW(static_cast<void>(File::open(freshPath("fileMissing"))), std::runtime_error);
}

TEST(File, resizeChangesTheLength)
{
  constexpr auto kInitialBytes = std::size_t{100};
  constexpr auto kShrunkBytes = std::size_t{40};
  constexpr auto kGrownBytes = std::size_t{400};
  const auto file = File::create(freshPath("fileResize"), kInitialBytes);
  file.resize(kShrunkBytes);
  EXPECT_EQ(file.size(), kShrunkBytes);
  file.resize(kGrownBytes);
  EXPECT_EQ(file.size(), kGrownBytes);
}

TEST(File, adoptedDescriptorHasNoPathAndClosesOnDestruction)
{
  const auto owner = File::create(freshPath("fileAdopt"), 16);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): fcntl is the POSIX file API.
  const auto descriptor = ::fcntl(owner.nativeHandle(), F_DUPFD_CLOEXEC, 0);
  ASSERT_GE(descriptor, 0);
  {
    const auto adopted = File{descriptor};
    EXPECT_TRUE(adopted.path().empty());
    EXPECT_EQ(adopted.nativeHandle(), descriptor);
    EXPECT_EQ(adopted.size(), 16U);
    EXPECT_EQ(adopted.describe(), "descriptor " + std::to_string(descriptor));
  }
  EXPECT_FALSE(isOpen(descriptor));
}

TEST(File, moveTransfersTheDescriptor)
{
  auto source = File::create(freshPath("fileMove"), 8);
  const auto descriptor = source.nativeHandle();
  const auto moved = File{std::move(source)};
  EXPECT_EQ(moved.nativeHandle(), descriptor);
  EXPECT_TRUE(isOpen(descriptor));
}

} // namespace nioc::containers
