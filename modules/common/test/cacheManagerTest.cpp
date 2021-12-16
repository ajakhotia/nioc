////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/common/cacheManager.hpp>
#include <gtest/gtest.h>
#include <cmath>

namespace naksh::common
{
namespace
{

std::string gTestCacheMessage;

class TestCache
{
public:
    explicit TestCache(int value) : mCached(value)
    {
        gTestCacheMessage = "Created cache for: " + std::to_string(mCached);
    }

    [[nodiscard]] bool validate(const int& value) const noexcept
    {
        return value == mCached;
    }

    int squareArea()
    {
        if (not mSquare)
        {
            gTestCacheMessage = "Computing for value: " + std::to_string(mCached);
            mSquare = std::pow(mCached, 2);
        }
        else
        {
            gTestCacheMessage = "Reusing for value: " + std::to_string(mCached);
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
    explicit TestClass(int value) : mValue(value)
    {

    }

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


} // End of anonymous namespace.


TEST(TestCache, construction)
{
    EXPECT_NO_THROW(TestCache(7));
    EXPECT_EQ(gTestCacheMessage, "Created cache for: 7");
}


TEST(TestCache, valid)
{
    const auto testCache = TestCache(13);
    EXPECT_EQ(gTestCacheMessage, "Created cache for: 13");
    EXPECT_TRUE(testCache.validate(13));
    EXPECT_FALSE(testCache.validate(0));
}


TEST(TestCache, squareArea)
{
    auto testCache = TestCache(17);

    EXPECT_EQ(gTestCacheMessage, "Created cache for: 17");

    EXPECT_EQ(289, testCache.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Computing for value: 17");

    EXPECT_EQ(289, testCache.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 17");
}


TEST(CacheManager, construction)
{
    gTestCacheMessage = "";
    EXPECT_NO_THROW(CacheManager<TestCache> cacheManager);
    EXPECT_EQ(gTestCacheMessage, "");
}


TEST(CacheManager, access)
{
    CacheManager<TestCache> cacheManager;
    {
        auto& cacheRef = cacheManager.access(19);

        EXPECT_EQ(gTestCacheMessage, "Created cache for: 19");
        EXPECT_EQ(361, cacheRef.squareArea());
        EXPECT_EQ(gTestCacheMessage, "Computing for value: 19");
    }

    {
        auto& cacheRef = cacheManager.access(19);
        EXPECT_EQ(361, cacheRef.squareArea());
        EXPECT_EQ(gTestCacheMessage, "Reusing for value: 19");
    }

    {
        auto& cacheRef = cacheManager.access(23);
        EXPECT_EQ(gTestCacheMessage, "Created cache for: 23");
        EXPECT_EQ(529, cacheRef.squareArea());
        EXPECT_EQ(gTestCacheMessage, "Computing for value: 23");
    }
}


TEST(Caching, construction)
{
    gTestCacheMessage = "";
    EXPECT_NO_THROW(TestClass(29));
    EXPECT_EQ(gTestCacheMessage, "");
}


TEST(Caching, squareArea)
{
    TestClass testClass(31);
    auto& internalIntRef = testClass.acquireReference();

    EXPECT_EQ(gTestCacheMessage, "");

    EXPECT_EQ(961, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Computing for value: 31");

    EXPECT_NE(800, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 31");

    EXPECT_EQ(961, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 31");


    internalIntRef = 37;
    EXPECT_EQ(1369, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Computing for value: 37");

    EXPECT_NE(1000, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 37");

    EXPECT_EQ(1369, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 37");


    internalIntRef = 41;
    EXPECT_EQ(1681, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Computing for value: 41");

    EXPECT_NE(1000, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 41");

    EXPECT_EQ(1681, testClass.squareArea());
    EXPECT_EQ(gTestCacheMessage, "Reusing for value: 41");
}


} // End of namespace naksh::common.

#pragma clang diagnostic pop
