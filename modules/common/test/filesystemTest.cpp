////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nioc/common/filesystem.hpp>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nioc::common
{
namespace fs = std::filesystem;

namespace
{

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

/// @brief Expect @p call to throw std::invalid_argument whose message contains @p subStr.
template<typename Call>
void expectInvalidArgumentErrorWithSubStr(Call&& call, const std::string_view subStr)
{
  try
  {
    std::forward<Call>(call)();
    FAIL() << "Expected std::invalid_argument mentioning " << subStr;
  }
  catch(const std::invalid_argument& error)
  {
    EXPECT_NE(std::string_view{error.what()}.find(subStr), std::string_view::npos) << error.what();
  }
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

TEST(RequireExistingDirectory, returnsRelativePathUnchanged)
{
  const auto dir = makeFreshEmptyDir("existing-relative");
  const auto relative = fs::relative(dir);
  EXPECT_EQ(requireExistingDirectory(relative), relative);
}

TEST(RequireExistingDirectory, acceptsSymlinkToDirectory)
{
  const auto dir = makeFreshEmptyDir("existing-symlink-target");
  const auto link = fs::temp_directory_path() / "nioc-filesystemTest" / "existing-symlink";
  fs::remove(link);
  fs::create_directory_symlink(dir, link);
  EXPECT_EQ(requireExistingDirectory(link), link);
}

TEST(RequireExistingDirectory, rejectsEmptyPath)
{
  EXPECT_THROW(requireExistingDirectory(fs::path{}), std::invalid_argument);
}

TEST(RequireExistingDirectory, rejectsDanglingSymlink)
{
  const auto dir = makeFreshEmptyDir("existing-dangling");
  const auto link = dir / "dangling";
  fs::create_symlink(dir / "gone", link);
  EXPECT_THROW(requireExistingDirectory(link), std::invalid_argument);
}

TEST(RequireExistingDirectory, diagnosticNamesThePath)
{
  const auto missing = fs::temp_directory_path() / "nioc-filesystemTest" / "existing-named";
  fs::remove_all(missing);
  expectInvalidArgumentErrorWithSubStr(
      [&] { requireExistingDirectory(missing); },
      missing.string());
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

TEST(RequireExistingFile, acceptsRegularFile)
{
  const auto dir = makeFreshEmptyDir("file-accepts");
  const auto file = dir / "file.txt";
  writeFile(file, "hello");
  EXPECT_EQ(requireExistingFile(file), file);
}

TEST(RequireExistingFile, acceptsEmptyFile)
{
  const auto dir = makeFreshEmptyDir("file-accepts-empty");
  const auto file = dir / "empty.txt";
  writeFile(file, "");
  EXPECT_EQ(requireExistingFile(file), file);
}

TEST(RequireExistingFile, returnsRelativePathUnchanged)
{
  const auto dir = makeFreshEmptyDir("file-relative");
  const auto file = dir / "file.txt";
  writeFile(file, "hello");
  const auto relative = fs::relative(file);
  EXPECT_EQ(requireExistingFile(relative), relative);
}

TEST(RequireExistingFile, acceptsSymlinkToFile)
{
  const auto dir = makeFreshEmptyDir("file-symlink");
  const auto file = dir / "file.txt";
  writeFile(file, "hello");
  const auto link = dir / "link.txt";
  fs::create_symlink(file, link);
  EXPECT_EQ(requireExistingFile(link), link);
}

TEST(RequireExistingFile, rejectsEmptyPath)
{
  EXPECT_THROW(requireExistingFile(fs::path{}), std::invalid_argument);
}

TEST(RequireExistingFile, rejectsDanglingSymlink)
{
  const auto dir = makeFreshEmptyDir("file-dangling");
  const auto link = dir / "dangling.txt";
  fs::create_symlink(dir / "gone.txt", link);
  EXPECT_THROW(requireExistingFile(link), std::invalid_argument);
}

TEST(RequireExistingFile, rejectsMissingPath)
{
  const auto missing = fs::temp_directory_path() / "nioc-filesystemTest" / "file-absent";
  fs::remove_all(missing);
  EXPECT_THROW(requireExistingFile(missing), std::invalid_argument);
}

TEST(RequireExistingFile, rejectsDirectory)
{
  const auto dir = makeFreshEmptyDir("file-rejects-dir");
  EXPECT_THROW(requireExistingFile(dir), std::invalid_argument);
}

TEST(RequireExistingFile, diagnosticNamesThePath)
{
  const auto dir = makeFreshEmptyDir("file-named");
  expectInvalidArgumentErrorWithSubStr([&] { requireExistingFile(dir); }, dir.string());
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

TEST(RequireEmptyDirectory, rejectsDirectoryHoldingOnlyASubdirectory)
{
  const auto dir = makeFreshEmptyDir("empty-rejects-subdir");
  fs::create_directory(dir / "child");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST(RequireEmptyDirectory, rejectsDirectoryHoldingOnlyAHiddenFile)
{
  const auto dir = makeFreshEmptyDir("empty-rejects-hidden");
  writeFile(dir / ".hidden", "");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST(RequireEmptyDirectory, acceptsSymlinkToEmptyDirectory)
{
  const auto dir = makeFreshEmptyDir("empty-symlink-target");
  const auto link = fs::temp_directory_path() / "nioc-filesystemTest" / "empty-symlink";
  fs::remove(link);
  fs::create_directory_symlink(dir, link);
  EXPECT_EQ(requireEmptyDirectory(link), link);
}

TEST(RequireEmptyDirectory, rejectsEmptyPath)
{
  EXPECT_THROW(requireEmptyDirectory(fs::path{}), std::invalid_argument);
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

TEST(RequireEmptyDirectory, diagnosticNamesThePath)
{
  const auto dir = makeFreshEmptyDir("empty-named");
  writeFile(dir / "file.txt", "hello");
  expectInvalidArgumentErrorWithSubStr([&] { requireEmptyDirectory(dir); }, dir.string());
}

} // namespace nioc::common
