////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>
#include <nioc/geometry/rigidTransform.hpp>

namespace nioc::geometry
{
namespace
{
class Alpha;
class Gamma;
class Echo;

} // namespace

TEST(RigidTransform, Construction)
{
    auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()));

    auto alphaFromGamma2 = RigidTransform<StaticFrame<Alpha>, DynamicFrame>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()), "Gamma");
}

TEST(RigidTransform, inverse)
{
    auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()));

    auto gammaFromAlpha = alphaFromGamma.inverse();
}

TEST(RigidTransform, Multiplication)
{
    auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()));

    auto gammaFromEcho = RigidTransform<StaticFrame<Gamma>, StaticFrame<Echo>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()));

    auto alphaFromEcho = alphaFromGamma * gammaFromEcho;

    std::cout << decltype(alphaFromEcho)::ParentFrame::name() << std::endl;
    std::cout << decltype(alphaFromEcho)::ChildFrame::name() << std::endl;
}

} // namespace nioc::geometry

#pragma clang diagnostic pop
