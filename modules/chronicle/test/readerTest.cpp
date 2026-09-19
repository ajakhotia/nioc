////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <nioc/chronicle/reader.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/utils.hpp>
#include <ranges>
#include <span>
#include <string_view>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace nioc::chronicle
{
namespace fs = std::filesystem;

static_assert(std::input_iterator<Reader::Iterator>);
static_assert(std::ranges::input_range<Reader>);

namespace
{

constexpr auto channelA = ChannelId{16983ULL};
constexpr auto channelB = ChannelId{68964786ULL};

std::vector<std::byte> makeBytes(const std::size_t size, const std::byte start = std::byte{0})
{
  auto bytes = std::vector<std::byte>(size);
  for(auto index = std::size_t{0}; index < size; ++index)
  {
    bytes.at(index) = std::byte(
        static_cast<unsigned char>(std::to_integer<unsigned char>(start) + index));
  }
  return bytes;
}

fs::path makeFreshEmptyDir(std::string_view name)
{
  const auto path = fs::temp_directory_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

/// @brief A fresh directory on the filesystem the test binary runs from, for tests that observe
/// the page cache: the temp directory is commonly tmpfs, where file pages have no backing store
/// and page-cache release is a no-op.
fs::path makeFreshDiskBackedDir(std::string_view name)
{
  const auto path = fs::current_path() / "nioc-chronicleTest" / name;
  fs::remove_all(path);
  fs::create_directories(path);
  return path;
}

/// @brief Pages of @p file present in the page cache, from mincore(2) over a private mapping,
/// after writing back any dirty pages so that release can take effect.
std::size_t cachedPageCount(const fs::path& file)
{
  const auto pageBytes = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  const auto byteCount = static_cast<std::size_t>(fs::file_size(file));
  const auto stream = std::unique_ptr<std::FILE, decltype(&std::fclose)>{
      std::fopen(file.c_str(), "rbe"),
      &std::fclose};
  if(stream == nullptr or ::fdatasync(::fileno(stream.get())) != 0)
  {
    ADD_FAILURE() << "Cannot open or sync " << file;
    return 0;
  }

  void* const address =
      ::mmap(nullptr, byteCount, PROT_READ, MAP_SHARED, ::fileno(stream.get()), 0);
  if(address == MAP_FAILED)
  {
    ADD_FAILURE() << "Cannot map " << file;
    return 0;
  }
  auto flags = std::vector<unsigned char>((byteCount + pageBytes - 1) / pageBytes);
  const auto status = ::mincore(address, byteCount, flags.data());
  static_cast<void>(::munmap(address, byteCount));
  if(status != 0)
  {
    ADD_FAILURE() << "mincore failed for " << file;
    return 0;
  }
  return static_cast<std::size_t>(
      std::ranges::count_if(flags, [](const auto flag) { return (flag & 1U) != 0U; }));
}

/// @brief Expect @p file to leave the page cache within a short bound: advice issued through the
/// ring completes asynchronously, so the last release may land after the Reader is gone.
void expectEventuallyUncached(const fs::path& file)
{
  constexpr auto kAttempts = 200;
  constexpr auto kPause = std::chrono::milliseconds{10};
  for(auto attempt = 0; attempt < kAttempts and cachedPageCount(file) != 0; ++attempt)
  {
    std::this_thread::sleep_for(kPause);
  }
  EXPECT_EQ(cachedPageCount(file), 0U) << file;
}

/// @brief The roll files of @p channel beneath @p logPath, in roll order.
std::vector<fs::path> rollFiles(const fs::path& logPath, const ChannelId channel)
{
  auto files = std::vector<fs::path>{};
  for(const auto& entry: fs::directory_iterator{logPath / common::hexString(channel.mValue)})
  {
    files.push_back(entry.path());
  }
  std::ranges::sort(files);
  return files;
}

void expectBytesEqual(const std::span<const std::byte> expected, std::span<const std::byte> actual)
{
  ASSERT_EQ(expected.size(), actual.size());
  const auto [expectedMismatch, actualMismatch] = std::ranges::mismatch(expected, actual);
  EXPECT_TRUE(expectedMismatch == expected.end())
      << "Bytes first differ at offset " << std::distance(expected.begin(), expectedMismatch);
}

/// @brief One record's expected identity: the channel it was written on and the seed its bytes
/// were generated from.
struct ExpectedRecord
{
  ChannelId mChannelId;
  std::size_t mSize{};
  std::byte mSeed{};
};

/// @brief A freshly written chronicle: where it lives and what its records should replay as.
struct WrittenChronicle
{
  fs::path mLogPath;
  std::vector<ExpectedRecord> mRecords;
};

/// @brief Write a chronicle whose records each carry distinct, index-derived bytes, so any
/// cross-record aliasing in replay fails the byte comparison.
///
/// @param name Directory name for the fresh chronicle.
///
/// @param rollCapacity Roll size passed to the Writer; small values force many rolls.
///
/// @param recordSizes Per-record byte counts, written round-robin over channelA and channelB.
WrittenChronicle writeDistinctRecords(
    const std::string_view name,
    const std::size_t rollCapacity,
    const std::vector<std::size_t>& recordSizes)
{
  auto expected = std::vector<ExpectedRecord>{};
  auto writer = Writer{makeFreshEmptyDir(name), rollCapacity};
  for(auto index = std::size_t{0}; index < recordSizes.size(); ++index)
  {
    const auto channelId = (index % 2 == 0) ? channelA : channelB;
    const auto seed = std::byte{static_cast<unsigned char>((index * 7) + 1)};
    writer.write(channelId, makeBytes(recordSizes.at(index), seed));
    expected.push_back({.mChannelId = channelId, .mSize = recordSizes.at(index), .mSeed = seed});
  }
  return {.mLogPath = writer.path(), .mRecords = std::move(expected)};
}

/// @brief Replay @p written under the given window budgets and verify every record's channel and
/// bytes.
void expectReplayMatches(
    const WrittenChronicle& written,
    const std::uint64_t readAheadBytes = Reader::kDefaultReadAheadBytes,
    const std::uint64_t trailBehindBytes = Reader::kDefaultTrailBehindBytes)
{
  auto index = std::size_t{0};
  for(const auto& entry: Reader{written.mLogPath, readAheadBytes, trailBehindBytes})
  {
    ASSERT_LT(index, written.mRecords.size());
    const auto& record = written.mRecords.at(index);
    EXPECT_EQ(record.mChannelId, entry.mChannelId);
    expectBytesEqual(makeBytes(record.mSize, record.mSeed), entry.mCrate.span());
    ++index;
  }
  EXPECT_EQ(index, written.mRecords.size());
}

/// @brief The process's current resident set size in bytes, from /proc/self/statm.
std::size_t residentSetBytes()
{
  auto stream = std::ifstream{"/proc/self/statm"};
  auto totalPages = std::size_t{0};
  auto residentPages = std::size_t{0};
  stream >> totalPages >> residentPages;
  return residentPages * static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
}

} // namespace

TEST(Reader, replaysRecordsInRecordedOrder)
{
  const auto written =
      writeDistinctRecords("reader-recordOrder", 256, std::vector<std::size_t>{20, 34, 20, 34});

  expectReplayMatches(written);
}

TEST(Reader, constructionRejectsMissingDirectory)
{
  const auto missing = fs::temp_directory_path() / "nioc-chronicleTest" / "absent";
  fs::remove_all(missing);
  EXPECT_THROW(Reader{missing}, std::invalid_argument);
}

TEST(Reader, constructionRejectsDirectoryWithoutTimeline)
{
  const auto bare = fs::temp_directory_path() / "nioc-chronicleTest" / "noTimeline";
  fs::remove_all(bare);
  fs::create_directories(bare);
  EXPECT_THROW(Reader{bare}, std::invalid_argument);
}

TEST(Reader, everyBudgetShapeReplaysManyRollsByteForByte)
{
  // 64-byte rolls force a roll per record or two; the oversized record exceeds every window
  // below, and the tiny budgets march the window across several rolls per channel.
  constexpr auto kRecordCount = std::size_t{40};
  constexpr auto kOversizedBytes = std::size_t{4096};
  auto sizes = std::vector<std::size_t>(kRecordCount, 48);
  sizes.at(kRecordCount / 2) = kOversizedBytes;
  const auto written = writeDistinctRecords("reader-budgetShapes", 64, sizes);

  // Windows a few rolls wide, with and without a trail-behind distance.
  constexpr auto kNarrowReadAheadBytes = std::uint64_t{256};
  constexpr auto kNarrowTrailBehindBytes = std::uint64_t{128};
  constexpr auto kWideReadAheadBytes = std::uint64_t{512};

  expectReplayMatches(written);
  expectReplayMatches(written, kNarrowReadAheadBytes, kNarrowTrailBehindBytes);
  expectReplayMatches(written, kWideReadAheadBytes, 0);
}

TEST(Reader, singleRecordChronicleReplaysUnderMinimalBudget)
{
  const auto written =
      writeDistinctRecords("reader-singleRecord", 64, std::vector<std::size_t>{32});

  expectReplayMatches(written, 64, 0);
}

TEST(Reader, beginResumesWhereIterationLeftOff)
{
  const auto written =
      writeDistinctRecords("reader-beginResumes", 256, std::vector<std::size_t>(6, 40));

  auto reader = Reader{written.mLogPath};
  auto first = reader.begin();
  ++first;

  // begin() consumes one record to position itself, so after two records the resumed iterator
  // stands on the third.
  const auto resumed = reader.begin();
  const auto& record = written.mRecords.at(2);
  EXPECT_EQ(record.mChannelId, resumed->mChannelId);
  expectBytesEqual(makeBytes(record.mSize, record.mSeed), resumed->mCrate.span());
}

TEST(Reader, cratesStayValidAfterRollRetirementAndReaderDestruction)
{
  // Rolls retire mid-replay under the tiny budget; every Crate must stay byte-perfect through
  // retirement and past the Reader's own destruction.
  const auto written =
      writeDistinctRecords("reader-crateLifetime", 64, std::vector<std::size_t>(30, 56));

  auto crates = std::vector<Crate>{};
  {
    auto reader = Reader{written.mLogPath, 128, 0};
    for(const auto& entry: reader)
    {
      crates.push_back(entry.mCrate);
    }
  }

  ASSERT_EQ(crates.size(), written.mRecords.size());
  for(auto index = std::size_t{0}; index < crates.size(); ++index)
  {
    const auto& record = written.mRecords.at(index);
    expectBytesEqual(makeBytes(record.mSize, record.mSeed), crates.at(index).span());
  }
}

TEST(Reader, emptyChronicleHasNoEntries)
{
  const auto written = writeDistinctRecords("reader-empty", 64, {});

  auto reader = Reader{written.mLogPath};
  EXPECT_TRUE(reader.begin() == reader.end());
  expectReplayMatches(written);
}

TEST(Reader, workingSetBudgetBoundsResidentMemory)
{
  // 32 MiB of payload replayed under a 3 MiB window: without the trailing trim the touched pages
  // alone would grow the resident set by the full payload size.
  constexpr auto kMebibyte = std::uint64_t{1024ULL * 1024ULL};
  constexpr auto kPageBytes = std::size_t{4096};
  const auto written = writeDistinctRecords(
      "reader-residentBound",
      4 * kMebibyte,
      std::vector<std::size_t>(512, 64ULL * 1024ULL));

  const auto baseline = residentSetBytes();
  auto peak = std::size_t{0};

  auto reader = Reader{written.mLogPath, 2 * kMebibyte, kMebibyte};
  auto pageTouchSink = std::uint64_t{0};
  for(const auto& entry: reader)
  {
    for(const auto byte: entry.mCrate.span() | std::views::stride(kPageBytes))
    {
      pageTouchSink += std::to_integer<std::uint64_t>(byte);
    }
    peak = std::max(peak, residentSetBytes());
  }
  static_cast<void>(pageTouchSink);

  // Generous headroom over the 3 MiB window for allocator and page-cache noise; the assertion
  // still fails decisively if the trim is broken (growth would exceed the full 32 MiB payload).
  EXPECT_LT(peak - baseline, 16 * kMebibyte)
      << "Resident set grew by " << (peak - baseline) << " bytes";
}

TEST(Reader, releasedStridesLeaveThePageCache)
{
  // 40 MiB over two channels in 4 MiB rolls: 5 rolls per channel, 8 MiB strides, so the first
  // four strides (rolls 0 to 3 of each channel) are released during replay while the fifth stays
  // open. Page-table eviction must precede the page-cache release for those pages to actually
  // leave the cache, which is what mincore observes.
  constexpr auto kRollBytes = std::size_t{4ULL * 1024ULL * 1024ULL};
  constexpr auto kRecordBytes = std::size_t{64ULL * 1024ULL};
  constexpr auto kRecordCount = std::size_t{640};
  constexpr auto kReleasedRollsPerChannel = std::size_t{4};

  const auto logPath = makeFreshDiskBackedDir("reader-pageCacheRelease");
  {
    auto writer = Writer{logPath, kRollBytes};
    const auto payload = makeBytes(kRecordBytes, std::byte{1});
    for(auto index = std::size_t{0}; index < kRecordCount; ++index)
    {
      writer.write((index % 2 == 0) ? channelA : channelB, payload);
    }
  }

  const auto rolls = std::array{rollFiles(logPath, channelA), rollFiles(logPath, channelB)};
  for(const auto& channelRolls: rolls)
  {
    ASSERT_EQ(channelRolls.size(), kReleasedRollsPerChannel + 1);
    for(const auto& roll: channelRolls)
    {
      ASSERT_GT(cachedPageCount(roll), 0U) << roll; // Freshly written: still cached.
    }
  }

  auto touched = std::uint64_t{0};
  for(const auto& entry: Reader{logPath, kRollBytes, 0})
  {
    touched += std::to_integer<std::uint64_t>(entry.mCrate.span().front());
  }
  EXPECT_GT(touched, 0U);

  for(const auto& channelRolls: rolls)
  {
    for(const auto& roll: channelRolls | std::views::take(kReleasedRollsPerChannel))
    {
      expectEventuallyUncached(roll);
    }
    EXPECT_GT(cachedPageCount(channelRolls.back()), 0U) << channelRolls.back();
  }
  fs::remove_all(logPath.parent_path());
}

} // namespace nioc::chronicle
