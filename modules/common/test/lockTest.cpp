////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/common/locked.hpp>

namespace naksh::common
{
namespace
{
// A lambda function to extract value from pointer types.
const auto valueExtractorHelper = [](const auto& value)
{
    return *value;
};

} // End of anonymous namespace.


TEST(Locked, LockedConstruction)
{
    // Default construction of trivial type.
    {
        const Locked<int> locked;
        EXPECT_EQ(0, locked);
    }

    // Construction of a trivial type with an initial value.
    {
        const Locked<int> locked(7);
        EXPECT_EQ(7, locked);
    }

    // Default construction of a non-trivial type.
    {
        const Locked<std::vector<int>> locked;
        EXPECT_EQ(0u, locked([](const auto& value) { return value.size(); }));
    }

    // Construction of a non-trivial type with an initial value.
    {
        const Locked<std::vector<int>> locked(std::initializer_list<int>({1, 2, 3, 4, 5}));
        EXPECT_EQ(5u, locked([](const auto& value) { return value.size(); }));
    }

    // Default construction of movable-only entities.
    {
        Locked<std::unique_ptr<int>> locked;
        EXPECT_EQ(nullptr, locked);
    }

    // Construction of movable-only entities with an initial value.
    {
        Locked<std::unique_ptr<int>> locked(std::make_unique<int>(7));
        EXPECT_EQ(7, locked(valueExtractorHelper));
    }
}


TEST(Locked, LockedConstExecution)
{
    // With cExecute.
    {
        const Locked<int> locked(7);
        const auto valueCopy = locked.cExecute([](const auto& value) { return value; });
        EXPECT_EQ(7, valueCopy);
    }

    // With execute. Should automatically resolve to the const-overload.
    {
        const Locked<int> locked(7);
        const auto valueCopy = locked.execute([](const auto& value) { return value; });
        EXPECT_EQ(7, valueCopy);
    }

    // With the ()-operator. Should automatically resolve to the const-overload
    {
        const Locked<int> locked(7);
        const auto valueCopy = locked([](const auto& value) { return value; });
        EXPECT_EQ(7, valueCopy);
    }

    // With ()-operator and const-pass-by-value semantics.
    {
        const Locked<int> locked(7);
        const auto valueCopy = locked([](const auto value) { return value; });
        EXPECT_EQ(7, valueCopy);
    }

    // With ()-operator and pass-by-value semantics.
    {
        const Locked<int> locked(7);
        const auto valueCopy = locked([](auto value) { return ++value; });
        EXPECT_EQ(8, valueCopy);
    }
}


TEST(Locked, LockedNonConstExecution)
{
    // Pass by l-value reference.
    {
        Locked<int> locked(7);
        const auto valueCopy = locked.execute(
            [](auto& value)
            {
                value += 9;
                return value;
            });
        EXPECT_EQ(16, valueCopy);
    }

    // Pass by forwarding reference.
    {
        Locked<int> locked(7);
        const auto valueCopy = locked.execute(
            [](auto&& value)
            {
                value += 9;
                return value;
            });
        EXPECT_EQ(16, valueCopy);
    }

    // Pass by l-value reference. Using the non-const operator().
    {
        Locked<int> locked(7);
        const auto valueCopy = locked(
            [](auto& value)
            {
                value += 9;
                return value;
            });
        EXPECT_EQ(16, valueCopy);
    }

    // Pass by forwarding reference. Using the non-const operator() overload
    {
        Locked<int> locked(7);
        const auto valueCopy = locked(
            [](auto&& value)
            {
                value += 9;
                return value;
            });
        EXPECT_EQ(16, valueCopy);
    }
}


TEST(Locked, LockedCopyAssignment)
{
    Locked<int> locked(12);
    EXPECT_EQ(12, locked);

    locked = 13;
    EXPECT_EQ(13, locked);
}


TEST(Locked, LockedMoveAssignment)
{
    Locked<std::unique_ptr<int>> locked;
    EXPECT_EQ(nullptr, locked);

    locked = std::make_unique<int>(7);
    EXPECT_EQ(7, locked(valueExtractorHelper));

    auto anotherPtr = std::make_unique<int>(13);

    // Line below is illegal because unique_ptr<> is not copyable and anotherPtr is an
    // l-value and hence cannot be implicitly moved.
    // locked = anotherPtr;

    locked = std::move(anotherPtr);
    EXPECT_EQ(13, locked(valueExtractorHelper));
}


TEST(Locked, LockedCopyExtraction)
{
    Locked<int> locked(12);
    const auto extractedValue = locked.copy();

    EXPECT_EQ(12, extractedValue);
    EXPECT_EQ(12, locked);
}


TEST(Locked, LockedMoveExtraction)
{
    Locked<std::unique_ptr<int>> locked(std::make_unique<int>(13));

    const auto extracted = locked.move();

    EXPECT_EQ(13, *extracted);
    EXPECT_EQ(nullptr, locked);
}


TEST(Locked, LockedEqualityCheck)
{
    Locked<int> locked(13);

    EXPECT_TRUE(locked == 13);
    EXPECT_TRUE(13 == locked);

    EXPECT_FALSE(locked == 23);
    EXPECT_FALSE(23 == locked);
}


TEST(Locked, LockedInEqualityCheck)
{
    Locked<int> locked(13);

    EXPECT_FALSE(locked != 13);
    EXPECT_FALSE(13 != locked);

    EXPECT_TRUE(locked != 23);
    EXPECT_TRUE(23 != locked);
}


TEST(Locked, LockedLesserThanCheck)
{
    Locked<int> locked(13);

    EXPECT_TRUE(locked < 14);
    EXPECT_FALSE(locked < 13);
    EXPECT_FALSE(locked < 12);

    EXPECT_TRUE(12 < locked);
    EXPECT_FALSE(13 < locked);
    EXPECT_FALSE(14 < locked);
}


TEST(Locked, LockedLesserThanOrEqualCheck)
{
    Locked<int> locked(13);

    EXPECT_TRUE(locked <= 14);
    EXPECT_TRUE(locked <= 13);
    EXPECT_FALSE(locked < 12);

    EXPECT_TRUE(12 <= locked);
    EXPECT_TRUE(13 <= locked);
    EXPECT_FALSE(14 <= locked);
}


TEST(Locked, LockedGreaterThanCheck)
{
    Locked<int> locked(13);

    EXPECT_FALSE(locked > 14);
    EXPECT_FALSE(locked > 13);
    EXPECT_TRUE(locked > 12);

    EXPECT_FALSE(12 > locked);
    EXPECT_FALSE(13 > locked);
    EXPECT_TRUE(14 > locked);
}


TEST(Locked, LockedGreaterThanOrEqualCheck)
{
    Locked<int> locked(13);

    EXPECT_FALSE(locked >= 14);
    EXPECT_TRUE(locked >= 13);
    EXPECT_TRUE(locked >= 12);

    EXPECT_FALSE(12 >= locked);
    EXPECT_TRUE(13 >= locked);
    EXPECT_TRUE(14 >= locked);
}

} // namespace naksh::common

#pragma clang diagnostic pop
