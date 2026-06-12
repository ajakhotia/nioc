////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>

namespace nioc::common
{
namespace fs = std::filesystem;

namespace
{

/// Create a fresh empty directory at a deterministic path under the system temp directory.
/// Any prior contents are wiped.
fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-filesystemTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

void writeFile(const fs::path& path, std::string_view contents)
{
  fs::create_directories(path.parent_path());
  auto out = std::ofstream(path);
  out << contents;
}

} // namespace

TEST(RequireExistingDirectory, acceptsExistingDirectory)
{
  const auto dir = makeFreshEmptyDir("existing-accepts");
  EXPECT_EQ(requireExistingDirectory(dir), dir);
}

TEST(RequireExistingDirectory, acceptsNonEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("existing-nonempty");
  writeFile(dir / "file.txt", "hello");
  EXPECT_EQ(requireExistingDirectory(dir), dir);
}

TEST(RequireExistingDirectory, rejectsMissingPath)
{
  const auto missing = fs::temp_directory_path() / "nioc-filesystemTest" / "absent";
  fs::remove_all(missing);
  EXPECT_THROW(requireExistingDirectory(missing), std::invalid_argument);
}

TEST(RequireExistingDirectory, rejectsRegularFile)
{
  const auto dir = makeFreshEmptyDir("existing-rejects-file");
  const auto file = dir / "file.txt";
  writeFile(file, "hello");
  EXPECT_THROW(requireExistingDirectory(file), std::invalid_argument);
}

TEST(RequireEmptyDirectory, acceptsEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("empty-accepts");
  EXPECT_EQ(requireEmptyDirectory(dir), dir);
}

TEST(RequireEmptyDirectory, rejectsNonEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("empty-rejects-nonempty");
  writeFile(dir / "file.txt", "hello");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST(RequireEmptyDirectory, rejectsMissingPath)
{
  const auto missing = fs::temp_directory_path() / "nioc-filesystemTest" / "empty-absent";
  fs::remove_all(missing);
  EXPECT_THROW(requireEmptyDirectory(missing), std::invalid_argument);
}

TEST(RequireEmptyDirectory, rejectsRegularFile)
{
  const auto dir = makeFreshEmptyDir("empty-rejects-file");
  const auto file = dir / "file.txt";
  writeFile(file, "hello");
  EXPECT_THROW(requireEmptyDirectory(file), std::invalid_argument);
}

} // namespace nioc::common
