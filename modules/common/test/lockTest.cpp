////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/common/locked.hpp>

namespace nioc::common
{
namespace
{
// A lambda function to extract value from pointer types.
const auto kValueExtractorHelper = [](const auto& value) { return *value; };

} // namespace

TEST(Locked, LockedConstruction)
{
  // Default construction of trivial type.
  {
    const auto locked = Locked<int>{};
    EXPECT_EQ(0, locked);
  }

  // Construction of a trivial type with an initial value.
  {
    const auto locked = Locked<int>{7};
    EXPECT_EQ(7, locked);
  }

  // Default construction of a non-trivial type.
  {
    const auto locked = Locked<std::vector<int>>{};
    EXPECT_EQ(0U, locked([](const auto& value) { return value.size(); }));
  }

  // Construction of a non-trivial type with an initial value.
  {
    const auto locked = Locked<std::vector<int>>{std::initializer_list<int>({1, 2, 3, 4, 5})};
    EXPECT_EQ(5U, locked([](const auto& value) { return value.size(); }));
  }

  // Default construction of movable-only entities.
  {
    const auto locked = Locked<std::unique_ptr<int>>{};
    EXPECT_EQ(nullptr, locked);
  }

  // Construction of movable-only entities with an initial value.
  {
    constexpr auto kStoredValue = 7;
    auto locked = Locked<std::unique_ptr<int>>{std::make_unique<int>(kStoredValue)};
    EXPECT_EQ(7, locked(kValueExtractorHelper));
  }
}

TEST(Locked, LockedConstExecution)
{
  // With cExecute.
  {
    const auto locked = Locked<int>{7};
    const auto valueCopy = locked.cExecute([](const auto& value) { return value; });
    EXPECT_EQ(7, valueCopy);
  }

  // With execute. Should automatically resolve to the const-overload.
  {
    const auto locked = Locked<int>{7};
    const auto valueCopy = locked.execute([](const auto& value) { return value; });
    EXPECT_EQ(7, valueCopy);
  }

  // With the ()-operator. Should automatically resolve to the const-overload
  {
    const auto locked = Locked<int>{7};
    const auto valueCopy = locked([](const auto& value) { return value; });
    EXPECT_EQ(7, valueCopy);
  }

  // With ()-operator and const-pass-by-value semantics.
  {
    const auto locked = Locked<int>{7};
    const auto valueCopy = locked([](const auto value) { return value; });
    EXPECT_EQ(7, valueCopy);
  }

  // With ()-operator and pass-by-value semantics.
  {
    const auto locked = Locked<int>{7};
    const auto valueCopy = locked([](auto value) { return ++value; });
    EXPECT_EQ(8, valueCopy);
  }
}

TEST(Locked, LockedNonConstExecution)
{
  constexpr auto kInitialValue = 7;
  constexpr auto kIncrement = 9;

  // Pass by l-value reference.
  {
    auto locked = Locked<int>{kInitialValue};
    const auto valueCopy = locked.execute(
        [](auto& value)
        {
          value += kIncrement;
          return value;
        });
    EXPECT_EQ(16, valueCopy);
  }

  // Pass by forwarding reference.
  {
    auto locked = Locked<int>{kInitialValue};
    const auto valueCopy = locked.execute(
        [](auto&& value)
        {
          value += kIncrement;
          return value;
        });
    EXPECT_EQ(16, valueCopy);
  }

  // Pass by l-value reference. Using the non-const operator().
  {
    auto locked = Locked<int>{kInitialValue};
    const auto valueCopy = locked(
        [](auto& value)
        {
          value += kIncrement;
          return value;
        });
    EXPECT_EQ(16, valueCopy);
  }

  // Pass by forwarding reference. Using the non-const operator() overload
  {
    auto locked = Locked<int>{kInitialValue};
    const auto valueCopy = locked(
        [](auto&& value)
        {
          value += kIncrement;
          return value;
        });
    EXPECT_EQ(16, valueCopy);
  }
}

TEST(Locked, LockedCopyAssignment)
{
  constexpr auto kInitialValue = 12;
  constexpr auto kReassignedValue = 13;
  auto locked = Locked<int>{kInitialValue};
  EXPECT_EQ(12, locked);

  locked = kReassignedValue;
  EXPECT_EQ(13, locked);
}

TEST(Locked, LockedMoveAssignment)
{
  constexpr auto kStoredValue = 7;
  constexpr auto kReassignedValue = 13;
  auto locked = Locked<std::unique_ptr<int>>{};
  EXPECT_EQ(nullptr, locked);

  locked = std::make_unique<int>(kStoredValue);
  EXPECT_EQ(7, locked(kValueExtractorHelper));

  auto anotherPtr = std::make_unique<int>(kReassignedValue);

  // Line below is illegal because unique_ptr<> is not copyable and anotherPtr is an
  // l-value and hence cannot be implicitly moved.
  // locked = anotherPtr;

  locked = std::move(anotherPtr);
  EXPECT_EQ(13, locked(kValueExtractorHelper));
}

TEST(Locked, LockedCopyExtraction)
{
  const auto locked = Locked<int>{12};
  const auto extractedValue = locked.copy();

  EXPECT_EQ(12, extractedValue);
  EXPECT_EQ(12, locked);
}

TEST(Locked, LockedMoveExtraction)
{
  constexpr auto kStoredValue = 13;
  auto locked = Locked<std::unique_ptr<int>>{std::make_unique<int>(kStoredValue)};

  const auto extracted = locked.move();

  EXPECT_EQ(13, *extracted);
  EXPECT_EQ(nullptr, locked);
}

TEST(Locked, LockedEqualityCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_TRUE(locked == 13);
  EXPECT_TRUE(13 == locked);

  EXPECT_FALSE(locked == 23);
  EXPECT_FALSE(23 == locked);
}

TEST(Locked, LockedInEqualityCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_FALSE(locked != 13);
  EXPECT_FALSE(13 != locked);

  EXPECT_TRUE(locked != 23);
  EXPECT_TRUE(23 != locked);
}

TEST(Locked, LockedLesserThanCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_TRUE(locked < 14);
  EXPECT_FALSE(locked < 13);
  EXPECT_FALSE(locked < 12);

  EXPECT_TRUE(12 < locked);
  EXPECT_FALSE(13 < locked);
  EXPECT_FALSE(14 < locked);
}

TEST(Locked, LockedLesserThanOrEqualCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_TRUE(locked <= 14);
  EXPECT_TRUE(locked <= 13);
  EXPECT_FALSE(locked < 12);

  EXPECT_TRUE(12 <= locked);
  EXPECT_TRUE(13 <= locked);
  EXPECT_FALSE(14 <= locked);
}

TEST(Locked, LockedGreaterThanCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_FALSE(locked > 14);
  EXPECT_FALSE(locked > 13);
  EXPECT_TRUE(locked > 12);

  EXPECT_FALSE(12 > locked);
  EXPECT_FALSE(13 > locked);
  EXPECT_TRUE(14 > locked);
}

TEST(Locked, LockedGreaterThanOrEqualCheck)
{
  const auto locked = Locked<int>{13};

  EXPECT_FALSE(locked >= 14);
  EXPECT_TRUE(locked >= 13);
  EXPECT_TRUE(locked >= 12);

  EXPECT_FALSE(12 >= locked);
  EXPECT_TRUE(13 >= locked);
  EXPECT_TRUE(14 >= locked);
}

} // namespace nioc::common
