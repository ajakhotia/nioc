////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "adviceRing.hpp"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/common/filesystem.hpp>
#include <string>
#include <unistd.h>
#include <vector>

namespace nioc::chronicle
{
namespace
{

/// The scratch file's size: eight advice chunks' worth of bytes.
constexpr auto kFileBytes = std::uint64_t{1024ULL * 1024ULL};

/// One WILLNEED request, matching the kernel's default per-file readahead limit.
constexpr auto kChunkBytes = std::uint32_t{128U * 1024U};

/// @brief A file at a given path filled with kFillByte plus its open descriptor. Owns and
/// closes the descriptor; the enclosing ScratchDirectory owns and deletes the file itself.
struct ScratchFile
{
  /// The byte value every position of the file holds; read-back assertions compare against it.
  static constexpr auto kFillByte = char{7};

  ScratchFile(std::filesystem::path path, const std::size_t byteCount):
    mPath{std::move(path)},
    mDescriptor{[&]
                {
                  auto stream = std::ofstream{mPath, std::ios::binary | std::ios::trunc};
                  const auto bytes = std::vector<char>(byteCount, kFillByte);
                  stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
                  stream.close();
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                  return ::open(mPath.c_str(), O_RDONLY | O_CLOEXEC);
                }()}
  {
  }

  ~ScratchFile()
  {
    static_cast<void>(::close(mDescriptor));
  }

  ScratchFile(const ScratchFile&) = delete;
  ScratchFile(ScratchFile&&) noexcept = delete;
  ScratchFile& operator=(const ScratchFile&) = delete;
  ScratchFile& operator=(ScratchFile&&) noexcept = delete;

  std::filesystem::path mPath;
  int mDescriptor;
};

/// @brief This test's own directory beneath @p base: `niocUnitTest/<Suite>.<test>`.
std::filesystem::path unitTestDirectory(
    const std::filesystem::path& base = std::filesystem::temp_directory_path())
{
  const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
  return base / "niocUnitTest" / (std::string{info->test_suite_name()} + "." + info->name());
}

// NOLINTNEXTLINE(misc-multiple-inheritance): the fixture is the test and its directory.
class AdviceRingTest: public common::ScratchDirectory, public ::testing::Test
{
public:
  AdviceRingTest(): ScratchDirectory{unitTestDirectory()} {}
};

} // namespace

TEST_F(AdviceRingTest, queueFullBurstsLeaveATwoSlotRingHealthy)
{
  // A two-slot ring forces the queue-full submit path on nearly every request.
  auto ring = AdviceRing{2};
  if(not ring.asynchronous())
  {
    GTEST_SKIP() << "io_uring is unavailable in this environment.";
  }

  const auto file = ScratchFile{path() / "advice.bin", kFileBytes};
  ASSERT_GE(file.mDescriptor, 0);

  for(auto offset = std::uint64_t{0}; offset < kFileBytes; offset += kChunkBytes)
  {
    ring.advise(
        file.mDescriptor,
        {.mBegin = offset, .mEnd = offset + kChunkBytes},
        POSIX_FADV_WILLNEED);
  }
  ring.submit();

  ring.advise(file.mDescriptor, {.mBegin = 0, .mEnd = kFileBytes}, POSIX_FADV_DONTNEED);
  ring.submit();

  EXPECT_TRUE(ring.asynchronous());
}

TEST_F(AdviceRingTest, synchronousFallbackAppliesAdviceWithoutARing)
{
  // Zero slots cannot be set up, so the ring must degrade to synchronous advice on construction.
  auto ring = AdviceRing{0};
  EXPECT_FALSE(ring.asynchronous());

  const auto file = ScratchFile{path() / "advice.bin", kFileBytes};
  ASSERT_GE(file.mDescriptor, 0);
  ring.advise(file.mDescriptor, {.mBegin = 0, .mEnd = kFileBytes}, POSIX_FADV_WILLNEED);
  ring.advise(file.mDescriptor, {.mBegin = 0, .mEnd = kFileBytes}, POSIX_FADV_DONTNEED);
  ring.submit();

  auto stream = std::ifstream{file.mPath, std::ios::binary};
  auto byte = char{};
  stream.read(&byte, 1);
  EXPECT_EQ(byte, ScratchFile::kFillByte);
}

TEST_F(AdviceRingTest, destructionWithOperationsInFlightLeavesFileReadable)
{
  const auto file = ScratchFile{path() / "advice.bin", kFileBytes};
  ASSERT_GE(file.mDescriptor, 0);

  {
    auto ring = AdviceRing{2};
    if(not ring.asynchronous())
    {
      GTEST_SKIP() << "io_uring is unavailable in this environment.";
    }
    for(auto offset = std::uint64_t{0}; offset < kFileBytes; offset += kChunkBytes)
    {
      ring.advise(
          file.mDescriptor,
          {.mBegin = offset, .mEnd = offset + kChunkBytes},
          POSIX_FADV_WILLNEED);
    }
    ring.submit();
  }

  auto stream = std::ifstream{file.mPath, std::ios::binary};
  auto byte = char{};
  stream.read(&byte, 1);
  EXPECT_EQ(byte, ScratchFile::kFillByte);
}

} // namespace nioc::chronicle
