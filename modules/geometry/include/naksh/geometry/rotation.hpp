////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/math/special_functions/sinc.hpp>

namespace naksh::geometry
{


/// @brief  Representation for rotation in 3D using Rodriguez parametrisation.
/// @tparam Scalar_ Floating point type to be used.
template<typename Scalar_ = double>
class Rotation3 : public Eigen::RotationBase<Rotation3<Scalar_>, 3>
{
    using Base = Eigen::RotationBase<Rotation3<Scalar_>, 3>;

public:
    using Scalar = typename Base::Scalar;
    using RotationMatrixType = typename Base::RotationMatrixType;
    using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
    using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
    using QuaternionType = Eigen::Quaternion<Scalar>;
    using AngleAxisType = Eigen::AngleAxis<Scalar>;

    Rotation3(const Scalar x, const Scalar y, const Scalar z) :
        mParameters({x, y, z})
    {
    }

    explicit Rotation3(const Vector3& rodriguezParameters) :
        mParameters(rodriguezParameters)
    {
    }

    Rotation3(const Scalar angle, const Vector3& axis) :
        mParameters(axis * angle)
    {
        if (axis.norm() != Scalar(1))
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided axis was not normalized.");
        }
    }

    explicit Rotation3(const QuaternionType& quaternion) :
        mParameters(Scalar(2) * quaternion.vec() / boost::math::sinc_pi(std::acos(quaternion.w())))
    {
        if (quaternion.norm() != 1.0)
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided quaternion was not normalized.");
        }
    }

    Rotation3(const Rotation3&) = default;

    Rotation3(Rotation3&&) noexcept = default;

    ~Rotation3() = default;

    Rotation3& operator=(const Rotation3&) = default;

    Rotation3& operator=(Rotation3&&) noexcept = default;


    Scalar angle() const
    {
        return mParameters.norm();
    }

    Vector3 axis() const
    {
        return mParameters.normalized();
    }

    const Vector3& cParameters() const noexcept
    {
        return parameters();
    }

    const Vector3& parameters() const noexcept
    {
        return mParameters;
    }

    Vector3& parameters() noexcept
    {
        return mParameters;
    }

    explicit operator QuaternionType() const
    {
        return QuaternionType(AngleAxisType());
    }

    explicit operator AngleAxisType() const
    {
        return AngleAxis(mParameters.norm(), mParameters.normalized());
    }

    inline RotationMatrixType toRotationMatrix() const
    {
        return matrix();
    }

    inline Matrix3 matrix() const
    {
        const auto& ax = axis();
        const auto an = angle();
        Matrix3 skew;
        skew <<
            Scalar(0.0),    -ax.z(),        +ax.y(),
            +ax.z(),        Scalar(0.0),    -ax.x(),
            -ax.y(),        +ax.x(),        Scalar(0.0);

        return Matrix3::Identity() +
               std::sin(an) * skew +
               (Scalar(1.0) - std::cos(an)) * skew * skew;
    }

    inline Rotation3 inverse() const
    {
        return {Scalar(-1) * cParameters()};
    }

private:
    /// Stores rotation using rodriguez parameterization. The rodriguez parameters for a rotation
    /// around an axis is given by the product of the unit vector along the axis and the angle of
    /// rotation about it in radians.
    Vector3 mParameters;
};

} // End of namespace naksh::geometry.


namespace Eigen::internal
{

template<typename Scalar_>
struct [[maybe_unused]] traits<naksh::geometry::Rotation3<Scalar_>>
{
    typedef Scalar_ Scalar;
};

} // End of namespace Eigen::internal.
