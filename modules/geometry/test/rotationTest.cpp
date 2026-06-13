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
void expectEq(const Eigen::Vector3d& lhs, const Eigen::Vector3d& rhs)
{
  EXPECT_DOUBLE_EQ(lhs.x(), rhs.x());
  EXPECT_DOUBLE_EQ(lhs.y(), rhs.y());
  EXPECT_DOUBLE_EQ(lhs.z(), rhs.z());
}

void expectEq(const Eigen::Vector3f& lhs, const Eigen::Vector3f& rhs)
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

  expectEq(mrp3RotViaAngleAxis.cParameters(), mrp3RotViaQuaternion.cParameters());
  expectEq(mrp3RotViaAngleAndAxis.cParameters(), mrp3RotViaQuaternion.cParameters());
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

  constexpr auto arbitraryTurnAngle = -1.77 * std::numbers::pi_v<double>;
  mrpEquivalenceOverRotationsAroundPrincipleAxes(arbitraryTurnAngle);
  mrpEquivalenceOverRotationsAroundPrincipleAxes(1.0 * std::numbers::pi_v<double>);
}

TEST(Rotation3, parameters)
{
  const auto params = Eigen::Vector3d(0.2, 0.3, 0.5);
  auto rotation = Rotation3<double>(params);
  expectEq(params, rotation.cParameters());

  // The mutable view writes through to the stored parameters.
  const auto reassigned = Eigen::Vector3d(0.7, 0.11, 0.13);
  rotation.parameters() = reassigned;
  expectEq(reassigned, rotation.cParameters());
}

TEST(Rotation3, data)
{
  constexpr auto paramX = 0.2;
  constexpr auto paramY = 0.3;
  constexpr auto paramZ = 0.5;
  auto rotation = Rotation3<double>({paramX, paramY, paramZ});

  // data() exposes the same storage the parameters view reads.
  EXPECT_EQ(rotation.data(), rotation.cParameters().data());

  constexpr auto reassignedX = 0.7;
  *rotation.data() = reassignedX;
  EXPECT_EQ(reassignedX, rotation.x());
}

TEST(Rotation3, components)
{
  constexpr auto paramX = 0.2;
  constexpr auto paramY = 0.3;
  constexpr auto paramZ = 0.5;

  auto test = Rotation3<double>({paramX, paramY, paramZ});
  EXPECT_EQ(paramX, test.x());
  EXPECT_EQ(paramY, test.y());
  EXPECT_EQ(paramZ, test.z());

  constexpr auto reassignedX = 0.7;
  constexpr auto reassignedY = 0.11;
  constexpr auto reassignedZ = 0.13;

  test.x() = reassignedX;
  EXPECT_EQ(reassignedX, test.x());
  test.y() = reassignedY;
  EXPECT_EQ(reassignedY, test.y());
  test.z() = reassignedZ;
  EXPECT_EQ(reassignedZ, test.z());

  const auto constTest = Rotation3<double>({paramX, paramY, paramZ});
  EXPECT_EQ(paramX, constTest.x());
  EXPECT_EQ(paramY, constTest.y());
  EXPECT_EQ(paramZ, constTest.z());

  // Operations below are illegal as constTest is const qualified.
  // constTest.x() = 0.7;
  // EXPECT_EQ(0.7, test.x());
  // constTest.y() = 0.11;
  // EXPECT_EQ(0.11, test.y());
  // constTest.z() = 0.13;
  // EXPECT_EQ(0.13, test.z());
}

TEST(Rotation3, angle)
{
  constexpr auto thirdTurn = 2.0 * std::numbers::pi_v<double> / 3.0;
  const auto rotation = Rotation3<double>(thirdTurn, Eigen::Vector3d::UnitY());
  EXPECT_NEAR(thirdTurn, rotation.angle(), 1e-12);

  // The zero rotation has zero angle.
  EXPECT_DOUBLE_EQ(0.0, Rotation3<double>(Eigen::Vector3d::Zero()).angle());
}

TEST(Rotation3, axis)
{
  const auto axis = Eigen::Vector3d(1.0, 2.0, -2.0).normalized();
  const auto rotation = Rotation3<double>(0.8, axis);

  const auto recovered = rotation.axis();
  EXPECT_NEAR(0.0, (axis - recovered).norm(), 1e-12);
}

TEST(Rotation3, inverse)
{
  const auto rotation = Rotation3<double>(0.9, Eigen::Vector3d(0.3, -0.4, 0.5).normalized());

  // A rotation composed with its inverse is the identity: zero parameters, zero angle.
  const auto identity = rotation * rotation.inverse();
  EXPECT_NEAR(0.0, identity.cParameters().norm(), 1e-12);
  EXPECT_NEAR(0.0, identity.angle(), 1e-12);
}

TEST(Rotation3, compositionAddsAnglesOnASharedAxis)
{
  const auto axis = Eigen::Vector3d::UnitZ().eval();
  const auto first = Rotation3<double>(0.4, axis);
  const auto second = Rotation3<double>(0.7, axis);

  const auto composed = second * first;
  EXPECT_NEAR(1.1, composed.angle(), 1e-12);
  EXPECT_NEAR(0.0, (axis - composed.axis()).norm(), 1e-12);
}

// The rotation algebra must compose the same way the underlying quaternions do: lhs * rhs applies
// rhs first, then lhs. RigidTransform chaining relies on exactly this order.
TEST(Rotation3, compositionMatchesQuaternionProduct)
{
  const auto quaternionA = Eigen::Quaterniond(
      Eigen::AngleAxisd(0.6, Eigen::Vector3d(1.0, 0.5, -0.3).normalized()));
  const auto quaternionB = Eigen::Quaterniond(
      Eigen::AngleAxisd(-0.9, Eigen::Vector3d(0.2, -1.0, 0.4).normalized()));

  const auto composed = Rotation3<double>(quaternionA) * Rotation3<double>(quaternionB);
  const auto expected = Rotation3<double>(Eigen::Quaterniond(quaternionA * quaternionB));

  EXPECT_NEAR(0.0, (expected.cParameters() - composed.cParameters()).norm(), 1e-12);
}

TEST(MapOfRotation3, construction)
{
  constexpr auto paramX = 0.1;
  constexpr auto paramY = 0.2;
  constexpr auto paramZ = 0.3;
  std::array<double, 3> data = {paramX, paramY, paramZ};
  auto map = Eigen::Map<Rotation3<double>>(data.data());

  EXPECT_EQ(paramX, map.x());
  constexpr auto reassignedX = 0.4;
  data[0] = reassignedX;
  EXPECT_EQ(reassignedX, map.x());
}

TEST(ConstMapOfRotation3, construction)
{
  const std::array<double, 3> data = {0.1, 0.2, 0.3};
  auto map = Eigen::Map<const Rotation3<double>>(data.data());
  EXPECT_EQ(0.1, map.x());
}

TEST(Assignment, RotationAndMap)
{
  constexpr auto paramX = 0.1;
  constexpr auto paramY = 0.2;
  constexpr auto paramZ = 0.3;
  std::array<double, 3> data = {paramX, paramY, paramZ};
  const auto map = Eigen::Map<Rotation3<double>>(data.data());
  const auto constMap = Eigen::Map<const Rotation3<double>>(data.data());
  constexpr auto quarterTurn = std::numbers::pi_v<double> / 2.0;
  auto rot3 = Rotation3<double>(quarterTurn, Eigen::Vector3d::UnitZ());
  auto rot4 = Rotation3<double>(quarterTurn, Eigen::Vector3d::UnitZ());

  rot3 = map;
  rot4 = constMap;
}


} // namespace nioc::geometry
