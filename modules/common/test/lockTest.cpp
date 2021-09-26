////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/common/locked.hpp>
#include <gtest/gtest.h>

namespace naksh::common
{

TEST(CommonTest, LockedConstruction)
{
    // Default construction of trivial type.
    {
        const Locked<int> locked;
        EXPECT_EQ(0, locked.copy());
        //EXPECT_EQ(0, locked.move()); // Illegal as locked is const and move is non-const qualified.
    }

    // Construction of a trivial type with an initial value.
    {
        const Locked<int> locked(7);
        EXPECT_EQ(7, locked.copy());
    }

    // Default construction of a non-trivial type.
    {
        const Locked<std::vector<int>> locked;
        EXPECT_EQ(0u, locked.copy().size());
    }

    // Construction of a non-trivial type with an initial value.
    {
        const Locked<std::vector<int>> locked(
                std::initializer_list<int>({1, 2, 3, 4, 5}));

        EXPECT_EQ(5u, locked.copy().size());
    }

    // Default construction of movable-only entities.
    {
        Locked<std::unique_ptr<int>> locked;
        EXPECT_EQ(nullptr, locked.move());
    }

    // Construction of movable-only entities with an initial value.
    {
        Locked<std::unique_ptr<int>> locked(std::make_unique<int>(7));
        EXPECT_EQ(7, *(locked.move()));
    }
}


TEST(CommonTest, NonConstOperator)
{
    Locked<int> lockedInt(7);
    int rhs = 13;

    // Run an operation on the lockedInt in a thread-safe manner.
    lockedInt.execute(
            [&](int& lhs)
            {
                lhs = rhs + 12;
            });

    EXPECT_EQ(25, lockedInt);

    // Run an operation on lockedInt but lhs is accepted into the operation by value.
    // Hence, we don't expect this operation to modify lockedInt. However, we expect the
    // result of the operation to be 15 + 13 + 29. Note the += in the operation below.
    const auto result = lockedInt.cExecute(
            [&](int lhs)
            {
                lhs += rhs + 29;
                return lhs;
            });

    EXPECT_EQ(25, lockedInt);
    EXPECT_EQ(67, result);
}


TEST(CommonTest, LockedOperatorConst)
{
    const Locked<int> lockedInt(12);

    // This is legal because we use a const-reference for the lambda parameter.
    const auto r1 = lockedInt(
            [](const int& lhs)
            {
                return lhs + 7;
            });

    EXPECT_EQ(19, r1);
    EXPECT_EQ(12, lockedInt);


    // This is legal because we use pass-by-value for the lambda parameter.
    const auto r2 = lockedInt(
            [](int lhs)
            {
                return lhs + 29;
            });

    EXPECT_EQ(41, r2);


    // This ill-legal because we use non-const-reference for the lambda parameter
    // which discards the const qualifier of lockedInt.
    // const auto r3 = lockedInt(
    //        [](int& lhs)
    //        {
    //            return lhs + 39;
    //        });
}


TEST(CommonTest, AssignmentCopySemantics)
{
    Locked<int> lockedInt(12);
    EXPECT_EQ(12, lockedInt.copy());

    //const int a = 13;

    lockedInt = 13;
    EXPECT_EQ(13, lockedInt.copy());

    /*
    Locked<std::unique_ptr<int>> lockedIntPtr(std::make_unique<int>(12));

    auto intPtr = std::make_unique<int>(29);

    // This is ill-legal because intPtr is an l-value and unique pointer are copyable.
    //lockedIntPtr = intPtr;
     */
}


TEST(CommonTest, AssignmentMoveSemantics)
{
    Locked<std::unique_ptr<int>> lockedIntPtr(std::make_unique<int>(12));

    // This is ill-legal because intPtr is an l-value and unique pointers are not copyable.
    // Additionally, l-values cannot be moved implicitly.
    //lockedIntPtr = intPtr;


    // This is legal because rhs is an r-value(a temporary) and can be moved implicitly.
    lockedIntPtr = std::make_unique<int>(23);
    EXPECT_EQ(23, *(lockedIntPtr.move()));


    // This is legal because we use std::move() to convert intPtr from l-value to r-value so that
    // it can be moved into lockedIntPtr.
    auto intPtr = std::make_unique<int>(29);
    lockedIntPtr = std::move(intPtr);
    EXPECT_EQ(29, *(lockedIntPtr.move()));
    EXPECT_EQ(nullptr, lockedIntPtr);
}


} // End of namespace naksh::common.
