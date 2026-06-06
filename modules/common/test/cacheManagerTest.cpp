////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <gtest/gtest.h>
#include <nioc/common/cacheManager.hpp>

namespace nioc::common
{
namespace
{
std::string& testCacheMessage()
{
  static std::string message;
  return message;
}

class TestCache
{
public:
  explicit TestCache(int value): mCached(value)
  {
    testCacheMessage() = "Created cache for: " + std::to_string(mCached);
  }

  [[nodiscard]] bool validate(const int& value) const noexcept
  {
    return value == mCached;
  }

  int squareArea()
  {
    if(not mSquare)
    {
      testCacheMessage() = "Computing for value: " + std::to_string(mCached);
      mSquare = std::pow(mCached, 2);
    }
    else
    {
      testCacheMessage() = "Reusing for value: " + std::to_string(mCached);
    }

    return *mSquare;
  }

private:
  int mCached;

  std::optional<int> mSquare;
};

class TestClass
{
public:
  explicit TestClass(int value): mValue(value) {}

  int squareArea() const
  {
    return mMangedCache.access(mValue).squareArea();
  }

  int& acquireReference()
  {
    return mValue;
  }


private:
  int mValue;

  mutable CacheManager<TestCache> mMangedCache;
};


} // namespace

TEST(TestCache, construction)
{
  EXPECT_NO_THROW(TestCache(7));
  EXPECT_EQ(testCacheMessage(), "Created cache for: 7");
}

TEST(TestCache, valid)
{
  const auto testCache = TestCache(13);
  EXPECT_EQ(testCacheMessage(), "Created cache for: 13");
  EXPECT_TRUE(testCache.validate(13));
  EXPECT_FALSE(testCache.validate(0));
}

TEST(TestCache, squareArea)
{
  constexpr auto kSideLength = 17;
  auto testCache = TestCache(kSideLength);

  EXPECT_EQ(testCacheMessage(), "Created cache for: 17");

  EXPECT_EQ(289, testCache.squareArea());
  EXPECT_EQ(testCacheMessage(), "Computing for value: 17");

  EXPECT_EQ(289, testCache.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 17");
}

TEST(CacheManager, construction)
{
  testCacheMessage() = "";
  EXPECT_NO_THROW(const CacheManager<TestCache> cacheManager);
  EXPECT_EQ(testCacheMessage(), "");
}

TEST(CacheManager, access)
{
  constexpr auto kFirstCacheKey = 19;
  constexpr auto kSecondCacheKey = 23;
  auto cacheManager = CacheManager<TestCache>{};
  {
    auto& cacheRef = cacheManager.access(kFirstCacheKey);

    EXPECT_EQ(testCacheMessage(), "Created cache for: 19");
    EXPECT_EQ(361, cacheRef.squareArea());
    EXPECT_EQ(testCacheMessage(), "Computing for value: 19");
  }

  {
    auto& cacheRef = cacheManager.access(kFirstCacheKey);
    EXPECT_EQ(361, cacheRef.squareArea());
    EXPECT_EQ(testCacheMessage(), "Reusing for value: 19");
  }

  {
    auto& cacheRef = cacheManager.access(kSecondCacheKey);
    EXPECT_EQ(testCacheMessage(), "Created cache for: 23");
    EXPECT_EQ(529, cacheRef.squareArea());
    EXPECT_EQ(testCacheMessage(), "Computing for value: 23");
  }
}

TEST(Caching, construction)
{
  testCacheMessage() = "";
  EXPECT_NO_THROW(TestClass(29));
  EXPECT_EQ(testCacheMessage(), "");
}

TEST(Caching, squareArea)
{
  constexpr auto kInitialValue = 31;
  constexpr auto kUpdatedValue = 37;
  constexpr auto kFinalValue = 41;
  auto testClass = TestClass{kInitialValue};
  auto& internalIntRef = testClass.acquireReference();

  EXPECT_EQ(testCacheMessage(), "");

  EXPECT_EQ(961, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Computing for value: 31");

  EXPECT_NE(800, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 31");

  EXPECT_EQ(961, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 31");


  internalIntRef = kUpdatedValue;
  EXPECT_EQ(1369, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Computing for value: 37");

  EXPECT_NE(1000, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 37");

  EXPECT_EQ(1369, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 37");


  internalIntRef = kFinalValue;
  EXPECT_EQ(1681, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Computing for value: 41");

  EXPECT_NE(1000, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 41");

  EXPECT_EQ(1681, testClass.squareArea());
  EXPECT_EQ(testCacheMessage(), "Reusing for value: 41");
}


} // namespace nioc::common
