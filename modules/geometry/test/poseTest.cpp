////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <gtest/gtest.h>
#include <nioc/common/typeTraits.hpp>
#include <nioc/geometry/pose.hpp>
#include <numbers>
#include <span>
#include <utility>

namespace nioc::geometry
{
namespace
{
// Position used to build the reference pose. Shared by the double and float
// test cases (the float case casts these to single precision).
constexpr auto kPositionX = 0.1;
constexpr auto kPositionY = 0.2;
constexpr auto kPositionZ = 0.3;

// Seven-element pose parameter vectors (four quaternion, three position) used as
// raw construction inputs, and the six-element vector that must be rejected.
constexpr auto kPoseParamsD = std::array{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
constexpr auto kPoseParamsF = std::array{0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F};
constexpr auto kShortPoseParamsD = std::array{0.1, 0.2, 0.3, 0.4, 0.5, 0.6};

// The reference pose is a quarter turn (pi/2 radians) about the Z axis.
constexpr auto kQuarterTurnD = std::numbers::pi_v<double> / 2.0;
constexpr auto kQuarterTurnF = std::numbers::pi_v<float> / 2.F;

// Half the rotation angle; its sine equals each non-zero quaternion component.
constexpr auto kHalfQuarterTurnD = std::numbers::pi_v<double> / 4.0;
constexpr auto kHalfQuarterTurnF = std::numbers::pi_v<float> / 4.F;

Eigen::AngleAxisd angleAxisD()
{
  return Eigen::AngleAxisd{kQuarterTurnD, Eigen::Vector3d::UnitZ()};
}

Eigen::AngleAxisf angleAxisF()
{
  return Eigen::AngleAxisf{kQuarterTurnF, Eigen::Vector3f::UnitZ()};
}

Eigen::Quaterniond quaternionD()
{
  return Eigen::Quaterniond{angleAxisD()};
}

Eigen::Quaternionf quaternionF()
{
  return Eigen::Quaternionf{angleAxisF()};
}

Eigen::Vector3d vec3D()
{
  return Eigen::Vector3d{kPositionX, kPositionY, kPositionZ};
}

Eigen::Vector3f vec3F()
{
  return Eigen::Vector3f{
      static_cast<float>(kPositionX),
      static_cast<float>(kPositionY),
      static_cast<float>(kPositionZ)};
}

// For a pi/2 rotation about Z the quaternion's sin and cos components are both
// sin(pi/4), so a single accessor serves both slots.
double sinPiBy4D()
{
  return std::sin(kHalfQuarterTurnD);
}

float sinPiBy4F()
{
  return std::sin(kHalfQuarterTurnF);
}

template<typename Scalar>
void poseParamCheck(
    const Pose<Scalar>& pose,
    typename Pose<Scalar>::Parameters params,
    const std::string& testName)
{
  // Normalize the quaternion portion of the params.
  {
    const auto norm = Scalar(std::sqrt(
        std::pow(params[0], Scalar(2)) + std::pow(params[1], Scalar(2)) +
        std::pow(params[2], Scalar(2)) + std::pow(params[3], Scalar(2))));

    auto range = std::span(params.begin(), 4U);
    std::transform(
        range.begin(),
        range.end(),
        range.begin(),
        [norm](const auto param) { return param / norm; });
  }

  const auto& poseParams = pose.cParameters();
  for(auto ii = 0U; ii < poseParams.size(); ++ii)
  {
    EXPECT_NEAR(poseParams.at(ii), params.at(ii), std::numeric_limits<Scalar>::epsilon())
        << "Index: " << ii << ", Test: " << testName
        << ", Scalar representation: " << common::prettyName<Scalar>();
  }
}

} // namespace

TEST(Pose, ConstructionFromQuaternionAndVector3)
{
  {
    EXPECT_NO_THROW(Pose<double>(quaternionD(), vec3D()));

    const auto test = Pose<double>{quaternionD(), vec3D()};
    poseParamCheck(
        test,
        {0.0, 0.0, sinPiBy4D(), sinPiBy4D(), kPositionX, kPositionY, kPositionZ},
        test_info_->name());
  }

  {
    EXPECT_NO_THROW(Pose<float>(quaternionF(), vec3F()));

    const auto test = Pose<float>{quaternionF(), vec3F()};
    poseParamCheck(
        test,
        {0.F,
         0.F,
         sinPiBy4F(),
         sinPiBy4F(),
         static_cast<float>(kPositionX),
         static_cast<float>(kPositionY),
         static_cast<float>(kPositionZ)},
        test_info_->name());
  }
}

// TODO(ajakhotia): Test with move semantics.
TEST(Pose, ConstrutionFromParameterArray)
{
  {
    const auto& paramArray = kPoseParamsD;
    EXPECT_NO_THROW((Pose<double>(paramArray)));

    const auto test = Pose<double>{paramArray};
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    const auto& paramArray = kPoseParamsF;
    EXPECT_NO_THROW((Pose<float>(paramArray)));

    const auto test = Pose<float>{paramArray};
    poseParamCheck(test, paramArray, test_info_->name());
  }
}

// TODO(ajakhotia): Test with move semantics.
TEST(Pose, ConstructionFromParamterSpan)
{
  {
    const auto& paramArray = kPoseParamsD;
    auto paramSpan = std::span(paramArray.data(), paramArray.size());

    EXPECT_NO_THROW((Pose<double>(paramSpan)));

    const auto test = Pose<double>{paramSpan};
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    auto paramArray = kPoseParamsF;
    auto paramSpan = std::span<float>(paramArray.data(), paramArray.size());

    EXPECT_NO_THROW((Pose<float>(paramSpan)));

    const auto test = Pose<float>{paramSpan};
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    auto paramArray = kShortPoseParamsD;
    auto paramSpan = std::span(paramArray.data(), paramArray.size());

    EXPECT_THROW((Pose<double>(paramSpan)), std::invalid_argument);
  }
}

TEST(Pose, Cast)
{
  const auto testD = Pose<double>{quaternionD(), vec3D()};
  auto testF = testD.cast<float>();
  poseParamCheck(
      testD,
      {0.0, 0.0, sinPiBy4D(), sinPiBy4D(), kPositionX, kPositionY, kPositionZ},
      test_info_->name());
  poseParamCheck(
      testF,
      {0.F,
       0.F,
       sinPiBy4F(),
       sinPiBy4F(),
       static_cast<float>(kPositionX),
       static_cast<float>(kPositionY),
       static_cast<float>(kPositionZ)},
      test_info_->name());
}

TEST(Pose, StreamOperator)
{
  const auto testD = Pose<double>{quaternionD(), vec3D()};

  auto stringStream = std::stringstream{};
  stringStream << testD << '\n';
  EXPECT_EQ(
      stringStream.str(),
      "{ Orientation:[       0,        0, 0.707107, "
      "0.707107], Position:[0.1, 0.2, 0.3] }\n");
}

TEST(PoseMap, mutableMapNormalizesTheBufferInPlace)
{
  // Construction normalizes the quaternion portion of the caller's own memory: (0,0,0,2) becomes
  // the identity quaternion in the buffer itself. The position is untouched.
  constexpr auto kScaledW = 2.0;
  constexpr auto kPosY = 2.0;
  constexpr auto kPosZ = 3.0;
  auto buffer = std::array{0.0, 0.0, 0.0, kScaledW, 1.0, kPosY, kPosZ};
  const auto map = Eigen::Map<Pose<double>>{std::span(buffer)};

  EXPECT_DOUBLE_EQ(1.0, buffer[3]);
  EXPECT_DOUBLE_EQ(1.0, map.cOrientation().norm());
  EXPECT_DOUBLE_EQ(1.0, buffer[4]);
  EXPECT_DOUBLE_EQ(2.0, buffer[5]);
  EXPECT_DOUBLE_EQ(3.0, buffer[6]);
}

TEST(PoseMap, constMapReadsTheBufferVerbatim)
{
  const auto buffer = std::array{0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 3.0};
  const auto map = Eigen::Map<const Pose<double>>{std::span(buffer)};

  EXPECT_DOUBLE_EQ(1.0, map.cOrientation().w());
  EXPECT_DOUBLE_EQ(1.0, map.cPosition().x());
  EXPECT_DOUBLE_EQ(2.0, map.cPosition().y());
  EXPECT_DOUBLE_EQ(3.0, map.cPosition().z());
}

TEST(PoseMap, constMapRejectsNonUnitQuaternion)
{
  // The read-only map cannot normalize the caller's memory, so it refuses a non-unit quaternion
  // instead.
  const auto buffer = std::array{0.0, 0.0, 0.0, 2.0, 1.0, 2.0, 3.0};
  EXPECT_THROW((Eigen::Map<const Pose<double>>{std::span(buffer)}), std::invalid_argument);
}

TEST(PoseMap, bothMapsRejectWrongSizeBuffers)
{
  constexpr auto kPosY = 2.0;
  auto shortBuffer = std::array{0.0, 0.0, 0.0, 1.0, 1.0, kPosY};
  EXPECT_THROW((Eigen::Map<Pose<double>>{std::span(shortBuffer)}), std::invalid_argument);
  EXPECT_THROW(
      (Eigen::Map<const Pose<double>>{std::span(std::as_const(shortBuffer))}),
      std::invalid_argument);
}

TEST(Pose, InverseComposesToIdentity)
{
  const auto pose = Pose<double>{
      Eigen::Quaterniond(Eigen::AngleAxisd(0.8, Eigen::Vector3d(0.3, -0.4, 0.5).normalized())),
      Eigen::Vector3d(1.0, -2.0, 0.5)};

  const auto identity = pose * pose.inverse();

  EXPECT_NEAR(0.0, identity.orientation().angularDistance(Eigen::Quaterniond::Identity()), 1e-12);
  EXPECT_NEAR(0.0, identity.position().norm(), 1e-12);
}

TEST(Pose, CompositionFollowsTheSe3GroupProduct)
{
  const auto lhs = Pose<double>{
      Eigen::Quaterniond(Eigen::AngleAxisd(0.6, Eigen::Vector3d(1.0, 0.5, -0.3).normalized())),
      Eigen::Vector3d(1.0, 2.0, 3.0)};
  const auto rhs = Pose<double>{
      Eigen::Quaterniond(Eigen::AngleAxisd(-0.9, Eigen::Vector3d(0.2, -1.0, 0.4).normalized())),
      Eigen::Vector3d(-0.5, 0.25, 4.0)};

  const auto composed = lhs * rhs;

  // q = qL * qR, p = qL * pR + pL.
  const auto expectedOrientation = (lhs.orientation() * rhs.orientation()).normalized();
  const auto expectedPosition = ((lhs.orientation() * rhs.position()) + lhs.position()).eval();

  EXPECT_NEAR(0.0, composed.orientation().angularDistance(expectedOrientation), 1e-12);
  EXPECT_NEAR(0.0, (composed.position() - expectedPosition).norm(), 1e-12);
}

} // namespace nioc::geometry
