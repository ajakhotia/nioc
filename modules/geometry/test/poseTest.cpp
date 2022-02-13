////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <naksh/common/typeTraits.hpp>
#include <naksh/geometry/constants.hpp>
#include <naksh/geometry/pose.hpp>
#include <span>

namespace naksh::geometry
{
namespace
{
const auto kAngleAxisD1 = Eigen::AngleAxisd(kPi<double> / 2.0, Eigen::Vector3d::UnitZ());
const auto kAngleAxisF1 = Eigen::AngleAxisf(kPi<float> / 2.f, Eigen::Vector3f::UnitZ());
const auto kQuaternionD1 = Eigen::Quaterniond(kAngleAxisD1);
const auto kQuaternionF1 = Eigen::Quaternionf(kAngleAxisF1);
const auto kVec3D1 = Eigen::Vector3d(0.1, 0.2, 0.3);
const auto kVec3F1 = Eigen::Vector3f(0.1f, 0.2f, 0.3f);

const auto sinPiBy4D = std::sin(kPi<double> / 4.0);
const auto sinPiBy4F = std::sin(kPi<float> / 4.f);
const auto cosPiBy4D = std::sin(kPi<double> / 4.0);
const auto cosPiBy4F = std::sin(kPi<float> / 4.f);


template<typename Scalar>
void poseParamCheck(const Pose<Scalar>& pose,
                    typename Pose<Scalar>::Parameters params,
                    const std::string& testName)
{
    // Normalize the quaternion portion of the params.
    {
        const Scalar norm =
            std::sqrt(std::pow(params[0], Scalar(2)) + std::pow(params[1], Scalar(2)) +
                      std::pow(params[2], Scalar(2)) + std::pow(params[3], Scalar(2)));

        auto range = std::span(params.begin(), 4U);
        std::transform(range.begin(),
                       range.end(),
                       range.begin(),
                       [norm](const auto param) { return param / norm; });
    }

    const auto& poseParams = pose.cParameters();
    for(auto ii = 0U; ii < poseParams.size(); ++ii)
    {
        EXPECT_NEAR(poseParams[ii], params[ii], std::numeric_limits<Scalar>::epsilon())
            << "Index: " << ii << ", Test: " << testName
            << ", Scalar representation: " << common::prettyName<Scalar>();
    }
}

} // End of anonymous namespace.


TEST(Pose, ConstructionFromQuaternionAndVector3)
{
    {
        EXPECT_NO_THROW(Pose<double>(kQuaternionD1, kVec3D1));

        const Pose<double> test(kQuaternionD1, kVec3D1);
        poseParamCheck(test, {0.0, 0.0, sinPiBy4D, cosPiBy4D, 0.1, 0.2, 0.3}, test_info_->name());
    }

    {
        EXPECT_NO_THROW(Pose<float>(kQuaternionF1, kVec3F1));

        const Pose<float> test(kQuaternionF1, kVec3F1);
        poseParamCheck(
            test, {0.f, 0.f, sinPiBy4F, cosPiBy4F, 0.1f, 0.2f, 0.3f}, test_info_->name());
    }
}


// TODO: Test with move semantics.
TEST(Pose, ConstrutionFromParameterArray)
{
    {
        std::array<double, 7> paramArray = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
        EXPECT_NO_THROW((Pose<double>(paramArray)));

        const Pose<double> test(paramArray);
        poseParamCheck(test, paramArray, test_info_->name());
    }

    {
        std::array<float, 7> paramArray = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
        EXPECT_NO_THROW((Pose<float>(paramArray)));

        const Pose<float> test(paramArray);
        poseParamCheck(test, paramArray, test_info_->name());
    }
}


// TODO: Test with move semantics.
TEST(Pose, ConstructionFromParamterSpan)
{
    {
        const std::array<double, 7> paramArray = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
        auto paramSpan = std::span(paramArray.data(), paramArray.size());

        // EXPECT_NO_THROW((Pose<double>(paramSpan)));

        const Pose<double> test(paramSpan);
        poseParamCheck(test, paramArray, test_info_->name());
    }

    {
        std::array<float, 7> paramArray = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
        auto paramSpan = std::span<float>(paramArray.data(), paramArray.size());

        // EXPECT_NO_THROW((Pose<float>(paramSpan)));

        const Pose<float> test(paramSpan);
        poseParamCheck(test, paramArray, test_info_->name());
    }

    {
        std::array<double, 6> paramArray = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
        auto paramSpan = std::span(paramArray.data(), paramArray.size());

        // EXPECT_THROW((Pose<double>(paramSpan)), std::invalid_argument);
    }
}


TEST(Pose, Cast)
{
    const Pose<double> testD(kQuaternionD1, kVec3D1);
    auto testF = testD.cast<float>();
    poseParamCheck(testD, {0.0, 0.0, sinPiBy4D, cosPiBy4D, 0.1, 0.2, 0.3}, test_info_->name());
    poseParamCheck(testF, {0.f, 0.f, sinPiBy4F, cosPiBy4F, 0.1f, 0.2f, 0.3f}, test_info_->name());
}


TEST(Pose, StreamOperator)
{
    const Pose<double> testD(kQuaternionD1, kVec3D1);

    std::stringstream stringStream;
    stringStream << testD << std::endl;
    EXPECT_EQ(stringStream.str(),
              "{ Orientation:[       0,        0, 0.707107, "
              "0.707107], Position:[0.1, 0.2, 0.3] }\n");
}

} // End of namespace naksh::geometry.
#pragma clang diagnostic pop
