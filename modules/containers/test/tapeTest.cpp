////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <latch>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/tape.hpp>
#include <numeric>
#include <ranges>
#include <span>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace nioc::containers
{
namespace
{
namespace fs = std::filesystem;

using IntArrayTape = Tape<std::array<int, 8>>;

// Compile-time guarantees: contiguous range with raw-pointer iterators, claim hands back a span,
// and const-ness reaches the pointee.
static_assert(std::ranges::contiguous_range<IntArrayTape>);
static_assert(std::is_same_v<IntArrayTape::iterator, int*>);
static_assert(std::is_same_v<decltype(std::declval<IntArrayTape&>().claim(1)), std::span<int>>);
static_assert(std::is_same_v<decltype(std::declval<IntArrayTape&>().emplace(0)), int*>);
static_assert(std::is_same_v<decltype(std::declval<IntArrayTape&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const IntArrayTape&>().data()), const int*>);

// shrink_to_fit is available when the storage can be resized (e.g. std::vector, MmapArray).
static_assert(requires(Tape<std::vector<int>> tape) { tape.shrink_to_fit(); });

fs::path freshPath(const std::string_view name)
{
  const auto directory = fs::temp_directory_path() / "nioc-containersTest";
  fs::create_directories(directory);
  const auto path = directory / name;
  fs::remove(path);
  return path;
}

TEST(Tape, claimReservesSlotsAndFillsToCapacity)
{
  auto tape = Tape<std::array<int, 4>>{};
  EXPECT_TRUE(tape.empty());
  EXPECT_EQ(tape.capacity(), 4U);

  const auto first = tape.claim(2);
  ASSERT_EQ(first.size(), 2U);
  first[0] = 1;
  first[1] = 2;

  const auto second = tape.claim(2);
  ASSERT_EQ(second.size(), 2U);
  second[0] = 3;
  second[1] = 4;

  EXPECT_TRUE(tape.full());
  EXPECT_EQ(tape.size(), 4U);

  const auto values = std::vector<int>(tape.begin(), tape.end());
  EXPECT_EQ(values, (std::vector<int>{1, 2, 3, 4}));
}

TEST(Tape, claimReturnsAnEmptySpanWhenItDoesNotFit)
{
  auto tape = Tape<std::array<int, 4>>{};

  ASSERT_EQ(tape.claim(3).size(), 3U);
  EXPECT_FALSE(tape.claim(2).data()); // 3 + 2 > 4: empty span, nothing reserved
  EXPECT_TRUE(tape.claim(2).empty());
  EXPECT_EQ(tape.size(), 3U);

  // A claim that still fits succeeds.
  EXPECT_EQ(tape.claim(1).size(), 1U);
  EXPECT_TRUE(tape.full());
}

TEST(Tape, claimLargerThanCapacityReturnsAnEmptySpan)
{
  auto tape = Tape<std::array<int, 4>>{};
  EXPECT_TRUE(tape.claim(5).empty());
  EXPECT_EQ(tape.size(), 0U);
}

TEST(Tape, emplaceConstructsInPlaceAndReportsFullWithNull)
{
  auto tape = Tape<std::array<int, 3>>{};

  auto* const first = tape.emplace(10);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(*first, 10);

  ASSERT_NE(tape.emplace(20), nullptr);
  ASSERT_NE(tape.emplace(30), nullptr);

  EXPECT_EQ(tape.emplace(40), nullptr);
  EXPECT_EQ(tape.size(), 3U);
  EXPECT_EQ(std::accumulate(tape.begin(), tape.end(), 0), 60);
}

TEST(Tape, adaptsAPreSizedVector)
{
  auto tape = Tape<std::vector<int>>{5};
  EXPECT_EQ(tape.capacity(), 5U);
  EXPECT_TRUE(tape.empty());

  const auto slots = tape.claim(3);
  std::iota(slots.begin(), slots.end(), 1); // 1, 2, 3

  EXPECT_EQ(tape.size(), 3U);
  EXPECT_EQ(std::accumulate(tape.begin(), tape.end(), 0), 6);
  EXPECT_EQ(tape.storage().size(), 5U); // the whole storage, including the unwritten tail
}

TEST(Tape, adaptsAMemoryMappedByteArray)
{
  const auto path = freshPath("tapeBytes");

  auto tape = Tape<MmapArray<std::byte>>{path, 16};
  EXPECT_EQ(tape.capacity(), 16U);

  const auto header = tape.claim(4);
  ASSERT_EQ(header.size(), 4U);
  std::ranges::fill(header, std::byte{0xAB});

  const auto body = tape.claim(8);
  ASSERT_EQ(body.size(), 8U);
  std::ranges::fill(body, std::byte{0xCD});

  EXPECT_EQ(tape.size(), 12U);
  EXPECT_EQ(tape.storage().size(), 16U);
  EXPECT_EQ(tape[0], std::byte{0xAB});
  EXPECT_EQ(tape[4], std::byte{0xCD});
  EXPECT_EQ(tape[11], std::byte{0xCD});
}

TEST(Tape, shrinkToFitTrimsCapacityToTheWrittenSize)
{
  auto tape = Tape<std::vector<int>>{8};
  EXPECT_EQ(tape.capacity(), 8U);

  const auto slots = tape.claim(3);
  std::iota(slots.begin(), slots.end(), 1); // 1, 2, 3

  tape.shrink_to_fit();
  EXPECT_EQ(tape.capacity(), 3U);
  EXPECT_EQ(tape.size(), 3U);
  EXPECT_TRUE(tape.full());
  EXPECT_EQ(std::accumulate(tape.begin(), tape.end(), 0), 6); // written elements survive
}

TEST(Tape, shrinkToFitTrimsTheBackingFile)
{
  const auto path = freshPath("tapeShrink");

  auto tape = Tape<MmapArray<std::byte>>{path, 16};
  const auto head = tape.claim(4);
  std::ranges::fill(head, std::byte{0xAB});

  tape.shrink_to_fit();
  EXPECT_EQ(tape.size(), 4U);          // the cursor is unchanged
  EXPECT_EQ(fs::file_size(path), 4U);  // the file is trimmed to the written bytes
  EXPECT_EQ(tape[0], std::byte{0xAB}); // the mapping stays, so written bytes are still readable
}

// Disjointness is an invariant, not a timing assertion: N concurrent single-slot claims into a
// capacity-N tape must yield exactly the indices {0..N-1} — no duplicates, no gaps — whatever the
// interleaving. (gtest ASSERT_* must not run inside the worker lambdas; record, then check after
// the threads join.)
TEST(Tape, concurrentClaimsAreDisjointAndComplete)
{
  constexpr std::size_t kThreads = 8;
  constexpr std::size_t kPerThread = 2000;
  constexpr std::size_t kCount = kThreads * kPerThread;

  auto tape = Tape<std::vector<int>>{kCount};
  auto seen = std::vector<std::vector<std::ptrdiff_t>>(kThreads);
  auto gate = std::latch{kThreads};

  {
    auto workers = std::vector<std::jthread>{};
    for(std::size_t thread = 0; thread < kThreads; ++thread)
    {
      workers.emplace_back(
          [&, thread]
          {
            gate.arrive_and_wait(); // release all threads together for real contention
            for(std::size_t i = 0; i < kPerThread; ++i)
            {
              const auto slot = tape.claim(1);
              seen[thread].push_back(slot.empty() ? -1 : slot.data() - tape.data());
            }
          });
    }
  } // jthreads join here

  auto offsets = std::vector<std::ptrdiff_t>{};
  for(const auto& perThread: seen)
  {
    offsets.insert(offsets.end(), perThread.begin(), perThread.end());
  }
  std::ranges::sort(offsets);

  auto expected = std::vector<std::ptrdiff_t>(kCount);
  std::iota(expected.begin(), expected.end(), 0);
  EXPECT_EQ(offsets, expected); // each slot claimed exactly once: disjoint and complete
}

// Variable-size claims under contention: the successful runs must tile [0, size()) with no gaps and
// no overlaps. This exercises the non-monotonic "full" path (a large claim fails while a smaller
// one still fits) and the empty-span sentinel.
TEST(Tape, concurrentVariableClaimsTileTheWrittenRegion)
{
  constexpr std::size_t kThreads = 8;
  constexpr std::size_t kAttempts = 1000;
  constexpr std::size_t kCapacity = 4000; // oversubscribed: demand far exceeds capacity

  auto tape = Tape<std::vector<int>>{kCapacity};
  auto runs = std::vector<std::vector<std::pair<std::ptrdiff_t, std::size_t>>>(kThreads);
  auto gate = std::latch{kThreads};

  {
    auto workers = std::vector<std::jthread>{};
    for(std::size_t thread = 0; thread < kThreads; ++thread)
    {
      workers.emplace_back(
          [&, thread]
          {
            gate.arrive_and_wait();
            for(std::size_t i = 0; i < kAttempts; ++i)
            {
              const auto count = 1U + (i % 4U); // 1..4
              const auto slot = tape.claim(count);
              if(not slot.empty())
              {
                runs[thread].emplace_back(slot.data() - tape.data(), slot.size());
              }
            }
          });
    }
  }

  auto intervals = std::vector<std::pair<std::ptrdiff_t, std::size_t>>{};
  for(const auto& perThread: runs)
  {
    intervals.insert(intervals.end(), perThread.begin(), perThread.end());
  }
  std::ranges::sort(intervals);

  auto next = std::ptrdiff_t{0};
  for(const auto& [offset, count]: intervals)
  {
    EXPECT_EQ(offset, next); // contiguous from 0 and disjoint
    next += static_cast<std::ptrdiff_t>(count);
  }
  EXPECT_EQ(static_cast<std::size_t>(next), tape.size());
  EXPECT_LE(tape.size(), kCapacity);
}

} // namespace
} // namespace nioc::containers
