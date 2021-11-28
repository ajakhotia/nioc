////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/common/cacheManager.hpp>
#include <gtest/gtest.h>
#include <cmath>

namespace naksh::common
{
namespace
{

class TestCache : public CacheBase<int>
{
public:
    explicit TestCache(int value) : mCached(value)
    {
        std::cout << "New test cache created with value: " << mCached << std::endl;
    }

    [[nodiscard]] bool valid(const int& value) const noexcept override
    {
        return value == mCached;
    }

    int squareArea()
    {
        if (not mSquare)
        {
            std::cout << "Computing square for value: " << mCached << std::endl;
            mSquare = std::pow(mCached, 2);
        }
        else
        {
            std::cout << "Reusing old results for value: " << mCached << std::endl;
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
    EXPECT_NO_THROW(const auto testCache = TestCache(7));
}


TEST(TestCache, valid)
{
    const auto testCache = TestCache(13);
    EXPECT_TRUE(testCache.valid(13));
    EXPECT_FALSE(testCache.valid(0));
}


TEST(TestCache, squareArea)
{
    auto testCache = TestCache(17);
    EXPECT_EQ(289, testCache.squareArea());
}


TEST(CacheManager, construction)
{
    EXPECT_NO_THROW(CacheManager<TestCache> cacheManager);
}


TEST(CacheManager, access)
{
    CacheManager<TestCache> cacheManager;
    {
        auto& cacheRef = cacheManager.access(19);
        EXPECT_EQ(361, cacheRef.squareArea());
    }

    {
        auto& cacheRef = cacheManager.access(19);
        EXPECT_EQ(361, cacheRef.squareArea());
    }

    {
        auto& cacheRef = cacheManager.access(23);
        EXPECT_EQ(529, cacheRef.squareArea());
    }
}


TEST(Caching, construction)
{
    EXPECT_NO_THROW(TestClass(29));
}


TEST(Caching, squareArea)
{
    TestClass testClass(31);
    auto& internalIntRef = testClass.acquireReference();

    std::cout << "Finished creation of test class. Computing squareArea from here on." << std::endl;
    EXPECT_EQ(961, testClass.squareArea());
    EXPECT_NE(800, testClass.squareArea());
    EXPECT_EQ(961, testClass.squareArea());

    internalIntRef = 37;
    std::cout << "Sneakily set value to 37." << std::endl;
    EXPECT_EQ(1369, testClass.squareArea());
    EXPECT_NE(1000, testClass.squareArea());
    EXPECT_EQ(1369, testClass.squareArea());

    internalIntRef = 41;
    std::cout << "Sneakily set value to 41." << std::endl;
    EXPECT_EQ(1681, testClass.squareArea());
    EXPECT_NE(1000, testClass.squareArea());
    EXPECT_EQ(1681, testClass.squareArea());
}


} // End of namespace naksh::common.

#pragma clang diagnostic pop
