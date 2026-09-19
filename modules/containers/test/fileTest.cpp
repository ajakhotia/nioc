////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <nioc/containers/file.hpp>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

namespace nioc::containers
{
namespace
{
namespace fs = std::filesystem;

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class FileTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  FileTest(): ScratchDirectory{unitTestDirectory()} {}
};

/// True when @p descriptor is still open in this process.
bool isOpen(const int descriptor)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): fcntl is the POSIX file API.
  return ::fcntl(descriptor, F_GETFD) != -1;
}

} // namespace

TEST_F(FileTest, createSizesAndReportsThePath)
{
  constexpr auto kBytes = std::size_t{1234};
  const auto path = this->path() / "nested/fileCreate";
  const auto file = File::create(path, kBytes);

  EXPECT_EQ(file.path(), path);
  EXPECT_GE(file.nativeHandle(), 0);
  EXPECT_EQ(file.size(), kBytes);
  EXPECT_EQ(fs::file_size(path), kBytes);
}

TEST_F(FileTest, createTruncatesAnExistingFile)
{
  const auto path = this->path() / "fileTruncate";
  static_cast<void>(File::create(path, 64));
  const auto file = File::create(path, 8);
  EXPECT_EQ(file.size(), 8U);
}

TEST_F(FileTest, openReadsAnExistingFile)
{
  constexpr auto kBytes = std::size_t{32};
  const auto path = this->path() / "fileOpen";
  static_cast<void>(File::create(path, kBytes));
  const auto file = File::open(path);
  EXPECT_EQ(file.size(), kBytes);
  EXPECT_EQ(file.path(), path);
}

TEST_F(FileTest, openingAMissingFileThrows)
{
  EXPECT_THROW(static_cast<void>(File::open(path() / "fileMissing")), std::runtime_error);
}

TEST_F(FileTest, resizeChangesTheLength)
{
  constexpr auto kInitialBytes = std::size_t{100};
  constexpr auto kShrunkBytes = std::size_t{40};
  constexpr auto kGrownBytes = std::size_t{400};
  const auto file = File::create(path() / "fileResize", kInitialBytes);
  file.resize(kShrunkBytes);
  EXPECT_EQ(file.size(), kShrunkBytes);
  file.resize(kGrownBytes);
  EXPECT_EQ(file.size(), kGrownBytes);
}

TEST_F(FileTest, adoptedDescriptorHasNoPathAndClosesOnDestruction)
{
  const auto owner = File::create(path() / "fileAdopt", 16);
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

TEST_F(FileTest, moveTransfersTheDescriptor)
{
  auto source = File::create(path() / "fileMove", 8);
  const auto descriptor = source.nativeHandle();
  const auto moved = File{std::move(source)};
  EXPECT_EQ(moved.nativeHandle(), descriptor);
  EXPECT_TRUE(isOpen(descriptor));
}

} // namespace nioc::containers
