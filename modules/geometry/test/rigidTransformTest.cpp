////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/geometry/rigidTransform.hpp>

namespace nioc::geometry
{
namespace
{
class Alpha;
class Gamma;
class Echo;

Pose<double> samplePose(
    const double angle,
    const Eigen::Vector3d& axis,
    const Eigen::Vector3d& position)
{
  return {Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis.normalized())), position};
}

void expectPosesNear(const Pose<double>& lhs, const Pose<double>& rhs)
{
  EXPECT_NEAR(0.0, lhs.orientation().angularDistance(rhs.orientation()), 1e-12);
  EXPECT_NEAR(0.0, (lhs.position() - rhs.position()).norm(), 1e-12);
}

} // namespace

TEST(RigidTransform, constructionStoresPoseAndFrames)
{
  const auto pose = samplePose(0.4, Eigen::Vector3d::UnitZ(), {1.0, 2.0, 3.0});

  const auto staticChild = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(pose);
  expectPosesNear(pose, staticChild.pose());

  const auto dynamicChild = RigidTransform<StaticFrame<Alpha>, DynamicFrame>(pose, "Gamma");
  expectPosesNear(pose, dynamicChild.pose());
  EXPECT_EQ("Gamma", dynamicChild.childFrame().name());
}

TEST(RigidTransform, inverseSwapsFramesAndInvertsPose)
{
  const auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
      samplePose(0.7, {0.3, -0.5, 0.8}, {1.0, -2.0, 0.5}));

  const auto gammaFromAlpha = alphaFromGamma.inverse();

  // The frame relationship flips with the pose.
  static_assert(std::is_same_v<
                std::remove_const_t<decltype(gammaFromAlpha)>,
                RigidTransform<StaticFrame<Gamma>, StaticFrame<Alpha>>>);

  // Transform times its inverse is the identity.
  expectPosesNear(
      samplePose(0.0, Eigen::Vector3d::UnitX(), Eigen::Vector3d::Zero()),
      (alphaFromGamma * gammaFromAlpha).pose());
}

TEST(RigidTransform, multiplicationComposesPosesAcrossTheSharedFrame)
{
  const auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
      samplePose(0.6, {1.0, 0.5, -0.3}, {1.0, 2.0, 3.0}));
  const auto gammaFromEcho = RigidTransform<StaticFrame<Gamma>, StaticFrame<Echo>>(
      samplePose(-0.9, {0.2, -1.0, 0.4}, {-0.5, 0.25, 4.0}));

  const auto alphaFromEcho = alphaFromGamma * gammaFromEcho;

  static_assert(std::is_same_v<
                std::remove_const_t<decltype(alphaFromEcho)>,
                RigidTransform<StaticFrame<Alpha>, StaticFrame<Echo>>>);

  // Composition follows the SE3 group product: q = qL * qR, p = qL * pR + pL.
  const auto& lhsPose = alphaFromGamma.pose();
  const auto& rhsPose = gammaFromEcho.pose();
  const auto expected = Pose<double>(
      (lhsPose.orientation() * rhsPose.orientation()).normalized(),
      (lhsPose.orientation() * rhsPose.position()) + lhsPose.position());

  expectPosesNear(expected, alphaFromEcho.pose());
}

TEST(RigidTransform, multiplicationRejectsMismatchedDynamicFrames)
{
  const auto pose = samplePose(0.0, Eigen::Vector3d::UnitX(), Eigen::Vector3d::Zero());

  const auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, DynamicFrame>(pose, "Gamma");
  const auto deltaFromEcho = RigidTransform<DynamicFrame, StaticFrame<Echo>>(pose, "Delta");

  // The shared frame carries runtime identities "Gamma" and "Delta"; composing across them is a
  // frame error.
  EXPECT_THROW(static_cast<void>(alphaFromGamma * deltaFromEcho), FrameCompositionException);
}

} // namespace nioc::geometry
