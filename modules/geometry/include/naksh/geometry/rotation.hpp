////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <naksh/common/cacheManager.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/math/special_functions/sinc.hpp>
#include <memory>

namespace naksh::geometry
{
namespace helpers
{


/// @brief  Computes the axis of rotation from the rodrigues parameters.
/// @tparam Scalar      Scalar type to use. Eg: float or double
/// @tparam Vector3     Vector3 type used to store the rodrigues parameters.
/// @param rodriguesParameters  Rodrigues parameters representing the rotation.
/// @return A unit vector along the axis of rotation represented by the rodriguesParameters
template<typename Scalar, typename Vector3 = Eigen::Matrix<Scalar, 3, 1>>
Vector3 computeAxis(const Vector3& rodriguesParameters)
{
    return rodriguesParameters.normalized();
}


/// @brief  Computes the angle of rotation from the rodrigues parameters.
/// @tparam Scalar      Scalar type to use. Eg: float or double
/// @tparam Vector3     Vector3 type used to store the rodrigues parameters.
/// @param rodriguesParameters  Rodrigues parameters representing the rotation.
/// @return The angle of rotation represented by the rodriguesParameters
template<typename Scalar, typename Vector3 = Eigen::Matrix<Scalar, 3, 1>>
Scalar computeAngle(const Vector3& rodriguesParameters)
{
    return rodriguesParameters.norm();
}


/// @brief  Computes a rotation matrix from the angle and axis of the rotation.
/// @tparam Scalar  Scalar type. Eg: float or double.
/// @tparam Vector3 Automatically computed Vector3 type.
/// @tparam Matrix3 Automatically computed Matrix3 type.
/// @param angle    Angle of the rotation.
/// @param axis     Axis of rotation. Must be a unit vector.
/// @return A 3x3 matrix representation corresponding rotation matrix.
template<
    typename Scalar,
    typename Vector3 = Eigen::Matrix<Scalar, 3, 1>,
    typename Matrix3 = Eigen::Matrix<Scalar, 3, 3>>
Matrix3 computeRotationMatrix(const Scalar angle, const Vector3& axis)
{
    if (axis.norm() != Scalar(1))
    {
        throw std::invalid_argument("[geometry::Rotation3] Provided axis was not normalized.");
    }

    Matrix3 skewSymmetricAxis;
    skewSymmetricAxis <<
        Scalar(0.0), -axis.z(), +axis.y(),
        +axis.z(), Scalar(0.0), -axis.x(),
        -axis.y(), +axis.x(), Scalar(0.0);

    return Matrix3::Identity() +
           std::sin(angle) * skewSymmetricAxis +
           (Scalar(1.0) - std::cos(angle)) * skewSymmetricAxis * skewSymmetricAxis;
}


/// @brief  Implementation of Rotation3<> using Rodrigues parameter that caches
///         and re-uses calculations on repeated calls. The cache is invalidated
///         if the rodrigues parameters stored with class do not match the one's
///         in the cache.
/// @tparam Scalar_ Scalar type to use. Eg: Float or double
template<typename Scalar_ = double>
class CachedRotation3 : public Eigen::RotationBase<CachedRotation3<Scalar_>, 3>
{
protected:
    using Base = Eigen::RotationBase<CachedRotation3<Scalar_>, 3>;

public:
    using Scalar = Scalar_;
    using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
    using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
    using RotationMatrix = Matrix3;
    using Quaternion = Eigen::Quaternion<Scalar>;
    using AngleAxis = Eigen::AngleAxis<Scalar>;

    using VectorType = Vector3;
    using RotationMatrixType = RotationMatrix;
    using QuaternionType = Quaternion;
    using AngleAxisType = AngleAxis;

    static_assert(std::is_same_v<Scalar, typename Base::Scalar>);
    static_assert(std::is_same_v<VectorType, typename Base::VectorType>);
    static_assert(std::is_same_v<RotationMatrixType, typename Base::RotationMatrixType>);


    CachedRotation3(const Scalar x, const Scalar y, const Scalar z) :
        mRodriguesParameters({x, y, z}),
        mManagedCache()
    {
    }

    explicit CachedRotation3(const Vector3& rodriguezParameters) :
        mRodriguesParameters(rodriguezParameters),
        mManagedCache()
    {
    }

    CachedRotation3(const Scalar angle, const Vector3& axis) :
        mRodriguesParameters(axis * angle),
        mManagedCache()
    {
        if (axis.norm() != Scalar(1))
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided axis was not normalized.");
        }
    }

    explicit CachedRotation3(const Quaternion& quaternion) :
        mRodriguesParameters(Scalar(2) * quaternion.vec() / boost::math::sinc_pi(std::acos(quaternion.w()))),
        mManagedCache()
    {
        if (quaternion.norm() != 1.0)
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided quaternion was not normalized.");
        }
    }

    CachedRotation3(const CachedRotation3&) = default;

    CachedRotation3(CachedRotation3&&) noexcept = default;

    ~CachedRotation3() = default;

    CachedRotation3& operator=(const CachedRotation3&) = default;

    CachedRotation3& operator=(CachedRotation3&&) noexcept = default;


    Scalar angle() const
    {
        return mManagedCache.template access(mRodriguesParameters).angle();
    }


    Vector3 axis() const
    {
        return mManagedCache.template access(mRodriguesParameters).axis();
    }


    const Vector3& cParameters() const noexcept
    {
        return parameters();
    }


    const Vector3& parameters() const noexcept
    {
        return mRodriguesParameters;
    }


    Vector3& parameters() noexcept
    {
        return mRodriguesParameters;
    }


    explicit operator Quaternion() const
    {
        return mManagedCache.template access(mRodriguesParameters).quaternion();
    }


    explicit operator AngleAxis() const
    {
        return mManagedCache.template access(mRodriguesParameters).angleAxis();
    }


    const RotationMatrix& toRotationMatrix() const
    {
        return matrix();
    }


    const Matrix3& matrix() const
    {
        return mManagedCache.template access(mRodriguesParameters).matrix();
    }


    CachedRotation3<Scalar> inverse() const
    {
        return {Scalar(-1) * mRodriguesParameters};
    }


private:

    class Cache : public common::CacheBase<Vector3>
    {
    public:

        explicit Cache(const Vector3& controlParameter) :
            common::CacheBase<Vector3>(),
            mCachedRodriguesParameters(controlParameter)
        {}

        Cache(const Cache&) = default;

        Cache(Cache&&) noexcept = default;

        ~Cache() = default;

        Cache& operator=(const Cache&) = default;

        Cache& operator=(Cache&&) noexcept = default;


        bool valid(const Vector3& controlParameter) const noexcept override
        {
            return controlParameter == mCachedRodriguesParameters;
        }

        Scalar angle()
        {
            if (not mAngle)
            {
                mAngle = computeAngle<Scalar>(mCachedRodriguesParameters);
            }

            return *mAngle;
        }

        const Vector3& axis()
        {
            if (not mAxis)
            {
                mAxis = computeAxis<Scalar>(mCachedRodriguesParameters);
            }

            return *mAxis;
        }

        const Matrix3& matrix()
        {
            if (not mMatrix)
            {
                mMatrix = computeRotationMatrix(angle(), axis());
            }

            return *mMatrix;
        }

        const AngleAxis& angleAxis()
        {
            if (not mAngleAxis)
            {
                mAngleAxis = AngleAxis(angle(), axis());
            }

            return *mAngleAxis;
        }

        const Quaternion& quaternion()
        {
            if (not mQuaternion)
            {
                mQuaternion = Quaternion(angleAxis());
            }

            return *mQuaternion;
        }

    private:
        Vector3 mCachedRodriguesParameters;

        std::optional<Scalar> mAngle;

        std::optional<Vector3> mAxis;

        std::optional<Matrix3> mMatrix;

        std::optional<AngleAxisType> mAngleAxis;

        std::optional<QuaternionType> mQuaternion;
    };


    /// Stores rotation using rodriguez parameterization. The rodriguez parameters for a rotation
    /// around an axis is given by the product of the unit vector along the axis and the angle of
    /// rotation about it in radians.
    Vector3 mRodriguesParameters;

    /// Managed cache to avoid computing results per each call of the accessors.
    mutable common::CacheManager<Cache> mManagedCache;
};



/// @brief  Implementation of Rotation3<> using Rodrigues parameter that does not
///         cache calculations.
/// @tparam Scalar_ Scalar type to use. Eg: Float or double
template<typename Scalar_ = double>
class VolatileRotation3 : public Eigen::RotationBase<VolatileRotation3<Scalar_>, 3>
{
protected:
    using Base = Eigen::RotationBase<VolatileRotation3<Scalar_>, 3>;

public:
    using Scalar = Scalar_;
    using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
    using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
    using RotationMatrix = Matrix3;
    using Quaternion = Eigen::Quaternion<Scalar>;
    using AngleAxis = Eigen::AngleAxis<Scalar>;

    using VectorType = Vector3;
    using RotationMatrixType = RotationMatrix;
    using QuaternionType = Quaternion;
    using AngleAxisType = AngleAxis;

    static_assert(std::is_same_v<Scalar, typename Base::Scalar>);
    static_assert(std::is_same_v<VectorType, typename Base::VectorType>);
    static_assert(std::is_same_v<RotationMatrixType, typename Base::RotationMatrixType>);


    VolatileRotation3(const Scalar x, const Scalar y, const Scalar z) :
        mRodriguesParameters({x, y, z})
    {
    }

    explicit VolatileRotation3(const Vector3& rodriguezParameters) :
        mRodriguesParameters(rodriguezParameters)
    {
    }

    VolatileRotation3(const Scalar angle, const Vector3& axis) :
        mRodriguesParameters(axis * angle)
    {
        if (axis.norm() != Scalar(1))
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided axis was not normalized.");
        }
    }

    explicit VolatileRotation3(const Quaternion& quaternion) :
        mRodriguesParameters(Scalar(2) * quaternion.vec() / boost::math::sinc_pi(std::acos(quaternion.w())))
    {
        if (quaternion.norm() != 1.0)
        {
            throw std::invalid_argument("[geometry::Rotation3] Provided quaternion was not normalized.");
        }
    }

    VolatileRotation3(const VolatileRotation3&) = default;

    VolatileRotation3(VolatileRotation3&&) noexcept = default;

    ~VolatileRotation3() = default;

    VolatileRotation3& operator=(const VolatileRotation3&) = default;

    VolatileRotation3& operator=(VolatileRotation3&&) noexcept = default;


    Scalar angle() const
    {
        return computeAngle(mRodriguesParameters);
    }


    Vector3 axis() const
    {
        return computeAxis(mRodriguesParameters);
    }


    const Vector3& cParameters() const noexcept
    {
        return parameters();
    }


    const Vector3& parameters() const noexcept
    {
        return mRodriguesParameters;
    }


    Vector3& parameters() noexcept
    {
        return mRodriguesParameters;
    }


    explicit operator Quaternion() const
    {

    }


    explicit operator AngleAxis() const
    {
        return {angle(), axis()};
    }


    RotationMatrix toRotationMatrix() const
    {
        return matrix();
    }


    Matrix3 matrix() const
    {
        return computeRotationMatrix(mRodriguesParameters);
    }


    VolatileRotation3<Scalar> inverse() const
    {
        return {Scalar(-1) * mRodriguesParameters};
    }


private:
    /// Stores rotation using rodriguez parameterization. The rodriguez parameters for a rotation
    /// around an axis is given by the product of the unit vector along the axis and the angle of
    /// rotation about it in radians.
    Vector3 mRodriguesParameters;
};


} // End of namespace helpers.



/// @brief  Representation for rotation in 3D using Rodriguez parametrisation.
/// @tparam Scalar_ Floating point type to be used.
template<
    typename Scalar_ = double, typename Implementation = helpers::CachedRotation3<Scalar_>>
class Rotation3 : public Implementation
{
    using Base [[maybe_unused]] = typename Implementation::Base;

public:
    using Scalar [[maybe_unused]] = Scalar_;
    using Vector3 [[maybe_unused]] = typename Implementation::Vector3;
    using Matrix3 [[maybe_unused]] = typename Implementation::Matrix3;
    using RotationMatrix [[maybe_unused]] = typename Implementation::RotationMatrix;
    using Quaternion [[maybe_unused]] = typename Implementation::Quaternion;
    using AngleAxis [[maybe_unused]] = typename Implementation::AngleAxis;

    using VectorType [[maybe_unused]] = typename Implementation::VectorType;
    using RotationMatrixType [[maybe_unused]] = typename Implementation::RotationMatrixType;
    using QuaternionType [[maybe_unused]] = typename Implementation::QuaternionType;
    using AngleAxisType [[maybe_unused]] = typename Implementation::AngleAxisType;

    template<typename... Args>
    explicit Rotation3(Args&& ... args): Implementation(std::forward<Args>(args)...)
    {
    }

    Rotation3(const Rotation3&) = default;

    Rotation3(Rotation3&&) noexcept = default;

    ~Rotation3() = default;

    Rotation3& operator=(const Rotation3&) = default;

    Rotation3& operator=(Rotation3&&) noexcept = default;
};


} // End of namespace naksh::geometry.


namespace Eigen::internal
{

template<typename Scalar_>
struct [[maybe_unused]] traits<naksh::geometry::helpers::CachedRotation3<Scalar_>>
{
    [[maybe_unused]] typedef Scalar_ Scalar;
};


template<typename Scalar_>
struct [[maybe_unused]] traits<naksh::geometry::helpers::VolatileRotation3<Scalar_>>
{
    [[maybe_unused]] typedef Scalar_ Scalar;
};


template<typename Scalar_>
struct [[maybe_unused]] traits<naksh::geometry::Rotation3<Scalar_>>
{
    [[maybe_unused]] typedef Scalar_ Scalar;
};


} // End of namespace Eigen::internal.
