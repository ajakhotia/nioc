////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/geometry/rotation.hpp>
#include <gtest/gtest.h>


namespace naksh::geometry
{

TEST(Rotation3, ConstructionFromXYZ)
{
    Rotation3<> rotation3D(0.0, 0.0, 0.0);
}

TEST(Rotation3, ConstructionFromQuaternion)
{
    const Eigen::Quaternion<double> quaternion(Eigen::AngleAxis<double>(M_PI/2.0, Eigen::Vector3d::UnitZ()));
    Rotation3<> rotation3D(quaternion);
    EXPECT_EQ(M_PI/2.0, rotation3D.angle());
    EXPECT_EQ(Eigen::Vector3d::UnitZ(), rotation3D.axis());
}

} // End of namespace naksh::messages.

#pragma clang diagnostic pop
