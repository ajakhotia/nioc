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
namespace
{

void expect_eq(const Eigen::Vector3d& lhs, const Eigen::Vector3d& rhs)
{
    EXPECT_DOUBLE_EQ(lhs.x(), rhs.x());
    EXPECT_DOUBLE_EQ(lhs.y(), rhs.y());
    EXPECT_DOUBLE_EQ(lhs.z(), rhs.z());
}


void expect_eq(const Eigen::Vector3f& lhs, const Eigen::Vector3f& rhs)
{
    EXPECT_FLOAT_EQ(lhs.x(), rhs.x());
    EXPECT_FLOAT_EQ(lhs.y(), rhs.y());
    EXPECT_FLOAT_EQ(lhs.z(), rhs.z());
}


template<typename Scalar>
void mrpEquivalenceTest(const Scalar angle, const Eigen::Matrix<Scalar, 3, 1>& axis)
{
    const Eigen::AngleAxis<Scalar> angleAxis(angle, axis);
    const Eigen::Quaternion<Scalar> quaternion(angleAxis);

    const Rotation3<Scalar> mrp3RotViaAngleAxis(angleAxis);
    const Rotation3<Scalar> mrp3RotViaQuaternion(quaternion);
    const Rotation3<Scalar> mrp3RotViaAngleAndAxis(angle, axis);

    expect_eq(mrp3RotViaAngleAxis.cParameters(), mrp3RotViaQuaternion.cParameters());
    expect_eq(mrp3RotViaAngleAndAxis.cParameters(), mrp3RotViaQuaternion.cParameters());
}


void mrpEquivalenceOverFloatRepresentations(const double angle, const Eigen::Vector3d& axis)
{
    mrpEquivalenceTest<float>(static_cast<float>(angle), axis.cast<float>());
    mrpEquivalenceTest<double>(angle, axis);
}


void mrpEquivalenceOverRotationsAroundPrincipleAxes(const double angle)
{
    mrpEquivalenceOverFloatRepresentations(angle, Eigen::Vector3d::UnitX());
    mrpEquivalenceOverFloatRepresentations(angle, Eigen::Vector3d::UnitY());
    mrpEquivalenceOverFloatRepresentations(angle, Eigen::Vector3d::UnitZ());
}


} // End of anonymous namespace.


TEST(Rotation3, construction)
{
    const auto angleAxis = Eigen::AngleAxisd(kPi<double>, Eigen::Vector3d::UnitZ());
    const auto quaternion = Eigen::Quaterniond(angleAxis);
    const auto vector3d = Eigen::Vector3d(0.0, 0.0, std::tan(kPi<double> / 4.0));

    EXPECT_NO_THROW((Rotation3<>(vector3d)));
    EXPECT_NO_THROW((Rotation3<>(angleAxis.angle(), angleAxis.axis())));
    EXPECT_NO_THROW((Rotation3<>(quaternion)));
    EXPECT_NO_THROW((Rotation3<>(angleAxis)));

    mrpEquivalenceOverRotationsAroundPrincipleAxes(-1.77 * kPi<double>);
    mrpEquivalenceOverRotationsAroundPrincipleAxes(1.0 * kPi<double>);
}


TEST(Rotation3, parameters)
{

}


TEST(Rotation3, data)
{

}


TEST(Rotation3, components)
{

}


TEST(Rotation3, angle)
{

}


TEST(Rotation3, axis)
{

}


TEST(Rotation3, inverse)
{

}


TEST(Rotation3, composition)
{

}


TEST(MapOfRotation3, construction)
{
    std::array<double, 3> data = {0.1, 0.2, 0.3};
    Eigen::Map<Rotation3<double>> map(data.data());

    EXPECT_EQ(0.1, map.x());
    data[0] = 0.4;
    EXPECT_EQ(0.4, map.x());

}


TEST(ConstMapOfRotation3, construction)
{
    const std::array<double, 3> data = {0.1, 0.2, 0.3};
    Eigen::Map<const Rotation3<double>> map(data.data());
    EXPECT_EQ(0.1, map.x());
}

} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
