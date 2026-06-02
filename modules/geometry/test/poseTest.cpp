////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/common/typeTraits.hpp>
#include <nioc/geometry/pose.hpp>
#include <numbers>
#include <span>

namespace nioc::geometry
{
namespace
{
const auto kAngleAxisD1 = Eigen::AngleAxisd(
    std::numbers::pi_v<double> / 2.0,
    Eigen::Vector3d::UnitZ());

const auto kAngleAxisF1 = Eigen::AngleAxisf(
    std::numbers::pi_v<float> / 2.F,
    Eigen::Vector3f::UnitZ());

const auto kQuaternionD1 = Eigen::Quaterniond(kAngleAxisD1);
const auto kQuaternionF1 = Eigen::Quaternionf(kAngleAxisF1);
const auto kVec3D1 = Eigen::Vector3d(0.1, 0.2, 0.3);
const auto kVec3F1 = Eigen::Vector3f(0.1F, 0.2F, 0.3F);

const auto kSinPiBy4D = std::sin(std::numbers::pi_v<double> / 4.0);
const auto kSinPiBy4F = std::sin(std::numbers::pi_v<float> / 4.F);
const auto kCosPiBy4D = std::sin(std::numbers::pi_v<double> / 4.0);
const auto kCosPiBy4F = std::sin(std::numbers::pi_v<float> / 4.F);

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
        [norm](const auto param)
        {
          return param / norm;
        });
  }

  const auto& poseParams = pose.cParameters();
  for(auto ii = 0U; ii < poseParams.size(); ++ii)
  {
    EXPECT_NEAR(poseParams[ii], params[ii], std::numeric_limits<Scalar>::epsilon())
        << "Index: " << ii << ", Test: " << testName
        << ", Scalar representation: " << common::prettyName<Scalar>();
  }
}

} // namespace

TEST(Pose, ConstructionFromQuaternionAndVector3)
{
  {
    EXPECT_NO_THROW(Pose<double>(kQuaternionD1, kVec3D1));

    const auto test = Pose<double>{ kQuaternionD1, kVec3D1 };
    poseParamCheck(test, { 0.0, 0.0, kSinPiBy4D, kCosPiBy4D, 0.1, 0.2, 0.3 }, test_info_->name());
  }

  {
    EXPECT_NO_THROW(Pose<float>(kQuaternionF1, kVec3F1));

    const auto test = Pose<float>{ kQuaternionF1, kVec3F1 };
    poseParamCheck(
        test,
        { 0.F, 0.F, kSinPiBy4F, kCosPiBy4F, 0.1F, 0.2F, 0.3F },
        test_info_->name());
  }
}

// TODO(ajakhotia): Test with move semantics.
TEST(Pose, ConstrutionFromParameterArray)
{
  {
    const std::array<double, 7> paramArray = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7 };
    EXPECT_NO_THROW((Pose<double>(paramArray)));

    const auto test = Pose<double>{ paramArray };
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    const std::array<float, 7> paramArray = { 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F };
    EXPECT_NO_THROW((Pose<float>(paramArray)));

    const auto test = Pose<float>{ paramArray };
    poseParamCheck(test, paramArray, test_info_->name());
  }
}

// TODO(ajakhotia): Test with move semantics.
TEST(Pose, ConstructionFromParamterSpan)
{
  {
    const std::array<double, 7> paramArray = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7 };
    auto paramSpan = std::span(paramArray.data(), paramArray.size());

    // EXPECT_NO_THROW((Pose<double>(paramSpan)));

    const auto test = Pose<double>{ paramSpan };
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    std::array<float, 7> paramArray = { 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F };
    auto paramSpan = std::span<float>(paramArray.data(), paramArray.size());

    // EXPECT_NO_THROW((Pose<float>(paramSpan)));

    const auto test = Pose<float>{ paramSpan };
    poseParamCheck(test, paramArray, test_info_->name());
  }

  {
    std::array<double, 6> paramArray = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 };
    auto paramSpan = std::span(paramArray.data(), paramArray.size());

    EXPECT_THROW((Pose<double>(paramSpan)), std::invalid_argument);
  }
}

TEST(Pose, Cast)
{
  const auto testD = Pose<double>{ kQuaternionD1, kVec3D1 };
  auto testF = testD.cast<float>();
  poseParamCheck(testD, { 0.0, 0.0, kSinPiBy4D, kCosPiBy4D, 0.1, 0.2, 0.3 }, test_info_->name());
  poseParamCheck(testF, { 0.F, 0.F, kSinPiBy4F, kCosPiBy4F, 0.1F, 0.2F, 0.3F }, test_info_->name());
}

TEST(Pose, StreamOperator)
{
  const auto testD = Pose<double>{ kQuaternionD1, kVec3D1 };

  auto stringStream = std::stringstream{};
  stringStream << testD << '\n';
  EXPECT_EQ(
      stringStream.str(),
      "{ Orientation:[       0,        0, 0.707107, "
      "0.707107], Position:[0.1, 0.2, 0.3] }\n");
}

} // namespace nioc::geometry
