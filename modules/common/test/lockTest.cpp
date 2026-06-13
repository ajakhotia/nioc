////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/common/locked.hpp>
#include <thread>
#include <vector>

namespace nioc::common
{
namespace
{
// A lambda function to extract value from pointer types.
const auto kValueExtractorHelper = [](const auto& value) { return *value; };

} // namespace

TEST(Locked, constructsValueInPlace)
{
  // Default construction of a trivial type.
  {
    const auto locked = Locked<int>{};
    EXPECT_EQ(0, locked);
  }

  // Construction of a trivial type with an initial value.
  {
    const auto locked = Locked<int>{7};
    EXPECT_EQ(7, locked);
  }

  // Construction of a non-trivial type with an initial value.
  {
    const auto locked = Locked<std::vector<int>>{std::initializer_list<int>({1, 2, 3, 4, 5})};
    EXPECT_EQ(5U, locked([](const auto& value) { return value.size(); }));
  }

  // Construction of move-only types.
  {
    constexpr auto kStoredValue = 7;
    auto locked = Locked<std::unique_ptr<int>>{std::make_unique<int>(kStoredValue)};
    EXPECT_EQ(7, locked(kValueExtractorHelper));
  }
}

TEST(Locked, constAccessReceivesConstReferenceAndReturnsOperationResult)
{
  const auto locked = Locked<int>{7};

  EXPECT_EQ(7, locked.cExecute([](const auto& value) { return value; }));

  // execute and operator() on a const wrapper resolve to the shared-lock overloads.
  EXPECT_EQ(7, locked.execute([](const auto& value) { return value; }));
  EXPECT_EQ(7, locked([](const auto& value) { return value; }));

  // An operation taking its parameter by value reads the protected value without touching it.
  EXPECT_EQ(8, locked([](auto value) { return ++value; }));
  EXPECT_EQ(7, locked);
}

TEST(Locked, mutableAccessReceivesMutableReferenceAndReturnsOperationResult)
{
  constexpr auto kInitialValue = 7;
  constexpr auto kIncrement = 9;

  auto locked = Locked<int>{kInitialValue};

  const auto viaExecute = locked.execute(
      [](auto& value)
      {
        value += kIncrement;
        return value;
      });
  EXPECT_EQ(16, viaExecute);

  const auto viaCallOperator = locked(
      [](auto& value)
      {
        value += kIncrement;
        return value;
      });
  EXPECT_EQ(25, viaCallOperator);
}

TEST(Locked, assignmentReplacesValue)
{
  constexpr auto kInitialValue = 12;
  constexpr auto kStoredValue = 7;
  constexpr auto kReassignedValue = 13;

  // Copy assignment.
  {
    auto locked = Locked<int>{kInitialValue};
    locked = kReassignedValue;
    EXPECT_EQ(kReassignedValue, locked);
  }

  // Move assignment of a move-only value.
  {
    auto locked = Locked<std::unique_ptr<int>>{};
    EXPECT_EQ(nullptr, locked);

    locked = std::make_unique<int>(kStoredValue);
    EXPECT_EQ(kStoredValue, locked(kValueExtractorHelper));

    auto anotherPtr = std::make_unique<int>(kReassignedValue);
    locked = std::move(anotherPtr);
    EXPECT_EQ(kReassignedValue, locked(kValueExtractorHelper));
  }
}

TEST(Locked, copyLeavesValueMoveTakesValue)
{
  constexpr auto kCopiedValue = 12;
  constexpr auto kMovedValue = 13;

  const auto locked = Locked<int>{kCopiedValue};
  EXPECT_EQ(kCopiedValue, locked.copy());
  EXPECT_EQ(kCopiedValue, locked);

  auto movable = Locked<std::unique_ptr<int>>{std::make_unique<int>(kMovedValue)};
  const auto extracted = movable.move();
  EXPECT_EQ(kMovedValue, *extracted);
  EXPECT_EQ(nullptr, movable);
}

TEST(Locked, comparisonsReflectProtectedValue)
{
  const auto locked = Locked<int>{13};

  EXPECT_TRUE(locked == 13 and 13 == locked);
  EXPECT_TRUE(locked != 23 and 23 != locked);

  EXPECT_TRUE(locked < 14 and 12 < locked);
  EXPECT_TRUE(locked <= 13 and 13 <= locked);
  EXPECT_TRUE(locked > 12 and 14 > locked);
  EXPECT_TRUE(locked >= 13 and 13 >= locked);

  EXPECT_FALSE(locked < 13 or 13 < locked);
  EXPECT_FALSE(locked > 13 or 13 > locked);
}

TEST(Locked, readersRunConcurrently)
{
  const auto locked = Locked<int>{7};
  auto readersInside = std::atomic_int{0};

  // Both readers wait inside their operation until the other one has entered. They can only meet
  // if the shared lock admits two readers at once; an exclusive implementation makes one of them
  // give up at the deadline and fail the expectation.
  const auto read = [&]
  {
    locked(
        [&readersInside](const auto& /*value*/)
        {
          readersInside.fetch_add(1);
          const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
          while(readersInside.load() < 2 and std::chrono::steady_clock::now() < deadline)
          {
            std::this_thread::yield();
          }
          EXPECT_EQ(2, readersInside.load());
        });
  };

  auto readerA = std::thread{read};
  auto readerB = std::thread{read};
  readerA.join();
  readerB.join();
}

TEST(Locked, writersNeverInterleave)
{
  constexpr auto kThreads = 4;
  constexpr auto kIncrementsPerThread = 100'000;

  auto locked = Locked<int>{0};

  // Plain int increments lose updates when two writers overlap, so an exact total proves every
  // write ran under the exclusive lock.
  const auto write = [&locked]
  {
    for(auto i = 0; i < kIncrementsPerThread; ++i)
    {
      locked([](auto& value) { ++value; });
    }
  };

  auto writers = std::vector<std::thread>{};
  writers.reserve(kThreads);
  for(auto i = 0; i < kThreads; ++i)
  {
    writers.emplace_back(write);
  }
  for(auto& writer: writers)
  {
    writer.join();
  }

  EXPECT_EQ(kThreads * kIncrementsPerThread, locked.copy());
}

} // namespace nioc::common
