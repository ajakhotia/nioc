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
#include <string>
#include <string_view>
#include <utility>

namespace nioc::common
{
namespace fs = std::filesystem;

namespace
{

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class RequireExistingDirectory: public common::ScratchDirectory, public ::testing::Test
{
public:
  RequireExistingDirectory(): ScratchDirectory{unitTestDirectory()} {}
};

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class RequireExistingFile: public common::ScratchDirectory, public ::testing::Test
{
public:
  RequireExistingFile(): ScratchDirectory{unitTestDirectory()} {}
};

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class RequireEmptyDirectory: public common::ScratchDirectory, public ::testing::Test
{
public:
  RequireEmptyDirectory(): ScratchDirectory{unitTestDirectory()} {}
};

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

TEST_F(RequireExistingDirectory, acceptsExistingDirectory)
{
  const auto& dir = path();
  EXPECT_EQ(requireExistingDirectory(dir), dir);
}

TEST_F(RequireExistingDirectory, acceptsNonEmptyDirectory)
{
  const auto& dir = path();
  writeFile(dir / "file.txt", "hello");
  EXPECT_EQ(requireExistingDirectory(dir), dir);
}

TEST_F(RequireExistingDirectory, returnsRelativePathUnchanged)
{
  const auto& dir = path();
  const auto relative = fs::relative(dir);
  EXPECT_EQ(requireExistingDirectory(relative), relative);
}

TEST_F(RequireExistingDirectory, acceptsSymlinkToDirectory)
{
  const auto target = path() / "target";
  fs::create_directories(target);
  const auto link = path() / "link";
  fs::create_directory_symlink(target, link);
  EXPECT_EQ(requireExistingDirectory(link), link);
}

TEST_F(RequireExistingDirectory, rejectsEmptyPath)
{
  EXPECT_THROW(requireExistingDirectory(fs::path{}), std::invalid_argument);
}

TEST_F(RequireExistingDirectory, rejectsDanglingSymlink)
{
  const auto link = path() / "dangling";
  fs::create_symlink(path() / "gone", link);
  EXPECT_THROW(requireExistingDirectory(link), std::invalid_argument);
}

TEST_F(RequireExistingDirectory, diagnosticNamesThePath)
{
  const auto missing = path() / "absent";
  expectInvalidArgumentErrorWithSubStr(
      [&] { requireExistingDirectory(missing); },
      missing.string());
}

TEST_F(RequireExistingDirectory, rejectsMissingPath)
{
  const auto missing = path() / "absent";
  EXPECT_THROW(requireExistingDirectory(missing), std::invalid_argument);
}

TEST_F(RequireExistingDirectory, rejectsRegularFile)
{
  const auto file = path() / "file.txt";
  writeFile(file, "hello");
  EXPECT_THROW(requireExistingDirectory(file), std::invalid_argument);
}

TEST_F(RequireExistingFile, acceptsRegularFile)
{
  const auto file = path() / "file.txt";
  writeFile(file, "hello");
  EXPECT_EQ(requireExistingFile(file), file);
}

TEST_F(RequireExistingFile, acceptsEmptyFile)
{
  const auto file = path() / "empty.txt";
  writeFile(file, "");
  EXPECT_EQ(requireExistingFile(file), file);
}

TEST_F(RequireExistingFile, returnsRelativePathUnchanged)
{
  const auto file = path() / "file.txt";
  writeFile(file, "hello");
  const auto relative = fs::relative(file);
  EXPECT_EQ(requireExistingFile(relative), relative);
}

TEST_F(RequireExistingFile, acceptsSymlinkToFile)
{
  const auto file = path() / "file.txt";
  writeFile(file, "hello");
  const auto link = path() / "link.txt";
  fs::create_symlink(file, link);
  EXPECT_EQ(requireExistingFile(link), link);
}

TEST_F(RequireExistingFile, rejectsEmptyPath)
{
  EXPECT_THROW(requireExistingFile(fs::path{}), std::invalid_argument);
}

TEST_F(RequireExistingFile, rejectsDanglingSymlink)
{
  const auto link = path() / "dangling.txt";
  fs::create_symlink(path() / "gone.txt", link);
  EXPECT_THROW(requireExistingFile(link), std::invalid_argument);
}

TEST_F(RequireExistingFile, rejectsMissingPath)
{
  const auto missing = path() / "absent";
  EXPECT_THROW(requireExistingFile(missing), std::invalid_argument);
}

TEST_F(RequireExistingFile, rejectsDirectory)
{
  const auto& dir = path();
  EXPECT_THROW(requireExistingFile(dir), std::invalid_argument);
}

TEST_F(RequireExistingFile, diagnosticNamesThePath)
{
  const auto& dir = path();
  expectInvalidArgumentErrorWithSubStr([&] { requireExistingFile(dir); }, dir.string());
}

TEST_F(RequireEmptyDirectory, acceptsEmptyDirectory)
{
  const auto& dir = path();
  EXPECT_EQ(requireEmptyDirectory(dir), dir);
}

TEST_F(RequireEmptyDirectory, rejectsNonEmptyDirectory)
{
  const auto& dir = path();
  writeFile(dir / "file.txt", "hello");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, rejectsDirectoryHoldingOnlyASubdirectory)
{
  const auto& dir = path();
  fs::create_directory(dir / "child");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, rejectsDirectoryHoldingOnlyAHiddenFile)
{
  const auto& dir = path();
  writeFile(dir / ".hidden", "");
  EXPECT_THROW(requireEmptyDirectory(dir), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, acceptsSymlinkToEmptyDirectory)
{
  const auto target = path() / "target";
  fs::create_directories(target);
  const auto link = path() / "link";
  fs::create_directory_symlink(target, link);
  EXPECT_EQ(requireEmptyDirectory(link), link);
}

TEST_F(RequireEmptyDirectory, rejectsEmptyPath)
{
  EXPECT_THROW(requireEmptyDirectory(fs::path{}), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, rejectsMissingPath)
{
  const auto missing = path() / "absent";
  EXPECT_THROW(requireEmptyDirectory(missing), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, rejectsRegularFile)
{
  const auto file = path() / "file.txt";
  writeFile(file, "hello");
  EXPECT_THROW(requireEmptyDirectory(file), std::invalid_argument);
}

TEST_F(RequireEmptyDirectory, diagnosticNamesThePath)
{
  const auto& dir = path();
  writeFile(dir / "file.txt", "hello");
  expectInvalidArgumentErrorWithSubStr([&] { requireEmptyDirectory(dir); }, dir.string());
}

namespace
{

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class ScratchDirectoryTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  ScratchDirectoryTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(ScratchDirectoryTest, constructionClearsAndCreatesTheDirectory)
{
  const auto target = path() / "owned";
  fs::create_directories(target / "stale");
  writeFile(target / "stale" / "leftover.txt", "old");

  const auto scratch = ScratchDirectory{target};

  EXPECT_EQ(scratch.path(), target);
  EXPECT_TRUE(fs::is_directory(target));
  EXPECT_TRUE(fs::is_empty(target));
}

TEST_F(ScratchDirectoryTest, constructionCreatesMissingParents)
{
  const auto target = path() / "a" / "b" / "c";
  const auto scratch = ScratchDirectory{target};
  EXPECT_TRUE(fs::is_directory(target));
}

TEST_F(ScratchDirectoryTest, destructionRemovesTheDirectoryAndItsContents)
{
  const auto target = path() / "owned";
  {
    const auto scratch = ScratchDirectory{target};
    writeFile(target / "nested" / "file.txt", "contents");
    ASSERT_TRUE(fs::exists(target / "nested" / "file.txt"));
  }
  EXPECT_FALSE(fs::exists(target));
}

} // namespace nioc::common
