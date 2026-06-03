////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/geometry/rotation.hpp>

namespace nioc::geometry
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
  const auto angleAxis = Eigen::AngleAxis<Scalar>(angle, axis);
  const auto quaternion = Eigen::Quaternion<Scalar>(angleAxis);

  const auto mrp3RotViaAngleAxis = Rotation3<Scalar>(angleAxis);
  const auto mrp3RotViaQuaternion = Rotation3<Scalar>(quaternion);
  const auto mrp3RotViaAngleAndAxis = Rotation3<Scalar>(angle, axis);

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


} // namespace

TEST(Rotation3, construction)
{
  const auto angleAxis = Eigen::AngleAxisd(std::numbers::pi_v<double>, Eigen::Vector3d::UnitZ());
  const auto quaternion = Eigen::Quaterniond(angleAxis);
  const auto vector3d = Eigen::Vector3d(0.0, 0.0, std::tan(std::numbers::pi_v<double> / 4.0));

  EXPECT_NO_THROW((Rotation3<>(vector3d)));
  EXPECT_NO_THROW((Rotation3<>(angleAxis.angle(), angleAxis.axis())));
  EXPECT_NO_THROW((Rotation3<>(quaternion)));
  EXPECT_NO_THROW((Rotation3<>(angleAxis)));

  mrpEquivalenceOverRotationsAroundPrincipleAxes(-1.77 * std::numbers::pi_v<double>);
  mrpEquivalenceOverRotationsAroundPrincipleAxes(1.0 * std::numbers::pi_v<double>);
}

TEST(Rotation3, parameters) {}

TEST(Rotation3, data) {}

TEST(Rotation3, components)
{
  auto test = Rotation3<double>({0.2, 0.3, 0.5});
  EXPECT_EQ(0.2, test.x());
  EXPECT_EQ(0.3, test.y());
  EXPECT_EQ(0.5, test.z());

  test.x() = 0.7;
  EXPECT_EQ(0.7, test.x());
  test.y() = 0.11;
  EXPECT_EQ(0.11, test.y());
  test.z() = 0.13;
  EXPECT_EQ(0.13, test.z());

  const auto constTest = Rotation3<double>({0.2, 0.3, 0.5});
  EXPECT_EQ(0.2, constTest.x());
  EXPECT_EQ(0.3, constTest.y());
  EXPECT_EQ(0.5, constTest.z());

  // Operations below are illegal as constTest is const qualified.
  // constTest.x() = 0.7;
  // EXPECT_EQ(0.7, test.x());
  // constTest.y() = 0.11;
  // EXPECT_EQ(0.11, test.y());
  // constTest.z() = 0.13;
  // EXPECT_EQ(0.13, test.z());
}

TEST(Rotation3, angle) {}

TEST(Rotation3, axis) {}

TEST(Rotation3, inverse) {}

TEST(Rotation3, composition) {}

TEST(MapOfRotation3, construction)
{
  std::array<double, 3> data = {0.1, 0.2, 0.3};
  auto map = Eigen::Map<Rotation3<double>>(data.data());

  EXPECT_EQ(0.1, map.x());
  data[0] = 0.4;
  EXPECT_EQ(0.4, map.x());
}

TEST(ConstMapOfRotation3, construction)
{
  const std::array<double, 3> data = {0.1, 0.2, 0.3};
  auto map = Eigen::Map<const Rotation3<double>>(data.data());
  EXPECT_EQ(0.1, map.x());
}

TEST(Assignment, RotationAndMap)
{
  std::array<double, 3> data = {0.1, 0.2, 0.3};
  const auto map = Eigen::Map<Rotation3<double>>(data.data());
  const auto constMap = Eigen::Map<const Rotation3<double>>(data.data());
  auto rot3 = Rotation3<double>(std::numbers::pi_v<double> / 2.0, Eigen::Vector3d::UnitZ());
  auto rot4 = Rotation3<double>(std::numbers::pi_v<double> / 2.0, Eigen::Vector3d::UnitZ());

  rot3 = map;
  rot4 = constMap;
}


} // namespace nioc::geometry
