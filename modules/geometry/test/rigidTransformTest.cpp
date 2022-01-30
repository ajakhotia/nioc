////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <naksh/geometry/rigidTransform.hpp>
#include <gtest/gtest.h>

namespace naksh::geometry
{
namespace
{

class Alpha;
class Gamma;
class Echo;

}

TEST(RigidTransform, Construction)
{
    auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero())
        );

    auto alphaFromGamma2 = RigidTransform<StaticFrame<Alpha>, DynamicFrame>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()),
        "Gamma");
}

TEST(RigidTransform, Multiplication)
{
    auto alphaFromGamma = RigidTransform<StaticFrame<Alpha>, StaticFrame<Gamma>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero())
    );

    auto gammaFromEcho = RigidTransform<StaticFrame<Gamma>, StaticFrame<Echo>>(
        Pose<double>(Eigen::Quaterniond::Identity(), Eigen::Vector3d::Zero()));

    auto alphaFromEcho = alphaFromGamma * gammaFromEcho;

    std::cout << decltype(alphaFromEcho)::ParentFrame::name() << std::endl;
    std::cout << decltype(alphaFromEcho)::ChildFrame::name() << std::endl;
}

} // End of namespace naksh::geometry.

#pragma clang diagnostic pop
