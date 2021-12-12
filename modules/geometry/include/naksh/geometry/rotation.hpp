////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <ostream>

namespace naksh::geometry
{

/// @brief  Provide access to pi in a modern C++ style.
/// @tparam Scalar  Scalar type to use.
template<typename Scalar>
constexpr const auto kPi = Scalar(M_PI);


template<typename Scalar_ = double>
class Rotation3;


/// @brief  Abstract rotation representation that leverages modified
///         rodrigues parametrisation. This representation has the benefits
///         of being compact. i.e. number of parameters equal number of degrees
///         of freedom.
///
///         Note that this representation does not handle rotations of magnitude
///         2(2k + 1) * pi properly. Do not use this class for representing
///         generic rotations. Eigen::Quaternion are better suited for that.
///         This representation is useful for parametrizing rotation that are
///         corrective in nature and have a small angle of rotation. This
///         parametrization appears to be stable for rotations with angle
///         lesser than 1.77 radians in magnitude.
///
///
///         Reading:
///             https://link.springer.com/article/10.1007/s10851-017-0765-x#Abs1
///
/// @tparam Derived Floating point representation to use.
template<typename Derived>
class Mrp3
{
public:
    static constexpr auto kDimensions = 3u;

    /// @brief  Provides pointer access to underlying storage.
    /// @return const Scalar* pointing to the underlying storage. The
    ///         storage is a 3-element array.
    inline decltype(auto) cData() const noexcept
    {
        return cDerived().cParameters().data();
    }


    /// @brief
    /// @return
    inline decltype(auto) data() const noexcept
    {
        return cData();
    }


    /// @brief
    /// @return
    inline decltype(auto) data() noexcept
    {
        return derived().parameters().data();
    }


    /// @brief
    /// @return
    inline decltype(auto) x() const noexcept
    {
        return cDerived().cParameters().x();
    }


    /// @brief
    /// @return
    inline decltype(auto) y() const noexcept
    {
        return cDerived().cParameters().y();
    }


    /// @brief
    /// @return
    inline decltype(auto) z() const noexcept
    {
        return cDerived().cParameters().z();
    }


    /// @brief
    /// @return
    inline decltype(auto) x() noexcept
    {
        return derived().parameters().x();
    }


    /// @brief
    /// @return
    inline decltype(auto) y() noexcept
    {
        return derived().parameters().y();
    }


    /// @brief
    /// @return
    inline decltype(auto) z() noexcept
    {
        return derived().parameters().z();
    }


    /// @brief
    /// @return
    inline decltype(auto) angle() const noexcept
    {
        using Scalar = typename Derived::Scalar;
        return Scalar(4) * std::atan(cDerived().cParameters().norm());
    }


    /// @brief
    /// @return
    inline decltype(auto) axis() const noexcept
    {
        return cDerived().cParameters().normalized().eval();
    }


    /// @brief
    /// @return
    inline decltype(auto) inverse() const
    {
        return Rotation3<typename Derived::Scalar>(
            typename Derived::Scalar(-1) * cDerived().cParameters());
    }


    /// @brief
    /// @param rhsBase
    /// @return
    inline decltype(auto) operator*(const Mrp3& rhsBase) const
    {
        using Scalar = typename Derived::Scalar;

        const auto& lhsMrp = this->cDerived().cParameters();
        const auto& rhsMrp = rhsBase.cDerived().cParameters();

        const auto lhsNorm2 = lhsMrp.squaredNorm();
        const auto rhsNorm2 = rhsMrp.squaredNorm();

        return Rotation3<Scalar>(
            ((Scalar(1) - rhsNorm2) * lhsMrp +
             (Scalar(1) - lhsNorm2) * rhsMrp +
             Scalar(2) * lhsMrp.cross(rhsMrp))
            /
            (Scalar(1) +
             lhsNorm2 * rhsNorm2 -
             Scalar(2) * lhsMrp.dot(rhsMrp)));
    }

protected:
    Mrp3() = default;

    Mrp3(const Mrp3&) noexcept = default;

    Mrp3(Mrp3&&) noexcept = default;

    ~Mrp3() = default;

    Mrp3& operator=(const Mrp3&) noexcept = default;

    Mrp3& operator=(Mrp3&&) noexcept = default;

private:

    /// @brief  Statically casts self to derived type.
    /// @return ConstRef to the derived type.
    inline constexpr const Derived& cDerived() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }


    /// @brief  Statically casts self to derived type.
    /// @return ConstRef to the derived type.
    inline constexpr const Derived& derived() const noexcept
    {
        return cDerived();
    }


    /// @brief  Statically casts self to derived type.
    /// @return NonConstRef to the derived type.
    inline constexpr Derived& derived() noexcept
    {
        return static_cast<Derived&>(*this);
    }
};


/// @brief  Stream insertion operator for Mrp3<>
/// @tparam Rotation    CRTP template parameter.
/// @param  stream       Output stream.
/// @param  mrp3         Input instance.
/// @return Modified stream.
template<typename Rotation>
std::ostream& operator<<(std::ostream& stream, const Mrp3<Rotation>& mrp3)
{
    static const auto ioFormat = Eigen::IOFormat(
        Eigen::FullPrecision,
        0, ", ", "\n", "", "", "[", "]");

    stream << mrp3.cDerived().cParameters().transpose().format(ioFormat);

    return stream;
}


/// @brief  Implementation of 3D rotation representation using modified
///         rodrigues parametrisation with Eigen::Matrix<..., 3, 1> storage.
///
/// @tparam Scalar_ Floating type to use.
template<typename Scalar_>
class Rotation3 : public Mrp3<Rotation3<Scalar_>>
{
public:
    using Base = Mrp3<Rotation3<Scalar_>>;

    static constexpr auto kDimensions = Base::kDimensions;

    using Scalar = Scalar_;

    using Vector3 = Eigen::Matrix<Scalar, kDimensions, 1>;

    using Matrix3 = Eigen::Matrix<Scalar, kDimensions, kDimensions>;

    using Quaternion = Eigen::Quaternion<Scalar>;

    using AngleAxis = Eigen::AngleAxis<Scalar>;

    using Parameters = Vector3;


    /// @brief  Constructs Rotation3 from triplet of modified rodrigues
    ///         parameters
    ///
    /// @param parameters   Modified rodrigues parameters laid out at
    ///                     {x, y, z} components.
    explicit Rotation3(const Vector3& parameters):
        mParameters(parameters)
    {
    }


    /// @brief  Constructs a Rotation3 equivalent to a rotation by angle
    ///         around an axis.
    /// @param angle    Angle of rotation.
    ///
    /// @param axis     Vector3 along the axis of rotation. The axis is
    ///                 normalized before use.
    Rotation3(const Scalar angle, const Vector3 axis):
        Rotation3(axis.normalized() * std::tan(angle / Scalar(4)))
    {
    }


    /// @brief  Constructs a Rotation3 equivalent to a rotation represented
    ///         by a quaternion.
    ///
    /// @param quaternion   Quaternion representing the rotation. The
    ///                     is normalized before use.
    explicit Rotation3(const Quaternion& quaternion):
        Rotation3(
            std::invoke(
                [](const Quaternion& normalizedQuaternion)
                {
                    assert(normalizedQuaternion.norm() == Scalar(1));
                    return (normalizedQuaternion.vec() / (Scalar(1) + normalizedQuaternion.w())).eval();
                },
                quaternion.normalized()))
    {
    }


    /// @brief  Constructs a Rotation equivalent to a rotation represented
    ///         by an angle-axis.
    ///
    /// @param angleAxis    Angle-axis representing the orientation.
    ///                     The axis component is normalized before use.
    explicit Rotation3(const AngleAxis& angleAxis): Rotation3(angleAxis.angle(), angleAxis.axis())
    {
    }


    Rotation3(const Rotation3&) = default;

    Rotation3(Rotation3&&) noexcept = default;

    ~Rotation3() = default;

    Rotation3& operator=(const Rotation3&) = default;

    Rotation3& operator=(Rotation3&&) noexcept = default;


    /// @brief
    /// @return
    inline const Parameters& cParameters() const noexcept { return mParameters; }


    /// @brief
    /// @return
    inline const Parameters& parameters() const noexcept { return cParameters(); }


    /// @brief
    /// @return
    inline Parameters& parameters() noexcept { return mParameters; }

private:
    Parameters mParameters;
};


} // End of namespace naksh::geometry.


/// @brief  Implementation of 3D rotation representation using modified
///         rodrigues parametrisation without owning storage. This class
///         leverages Eigen::Map functionality to create a Vector3 like
///         view of the underlying data array.
///
/// @tparam Scalar_     Floating type to use.
///
/// @tparam options_    Options to convey storage alignment.
///
template<typename Scalar_, int options_>
class Eigen::Map<naksh::geometry::Rotation3<Scalar_>, options_> :
    public naksh::geometry::Mrp3<Map<naksh::geometry::Rotation3<Scalar_>, options_>>
{
public:
    using Base = naksh::geometry::Mrp3<Map<naksh::geometry::Rotation3<Scalar_>, options_>>;

    static constexpr auto kDimensions = Base::kDimensions;

    using Scalar = Scalar_;

    using Vector3 [[maybe_unused]] = Matrix<Scalar, kDimensions, 1>;

    using Matrix3 [[maybe_unused]] = Matrix<Scalar, kDimensions, kDimensions>;

    using Parameters = Map<Vector3, options_>;


    /// @brief
    /// @param ptr
    explicit Map(Scalar* ptr): mParameters(ptr) {}

    Map(const Map&) = default;

    Map(Map&&) noexcept = default;

    ~Map() = default;

    Map& operator=(const Map&) = default;

    Map& operator=(Map&&) noexcept = default;


    /// @brief
    /// @return
    inline const Parameters& cParameters() const noexcept { return mParameters; }


    /// @brief
    /// @return
    inline const Parameters& parameters() const noexcept { return cParameters(); }


    /// @brief
    /// @return
    inline Parameters& parameters() noexcept { return mParameters; }

private:
    Parameters mParameters;
};


/// @brief  Implementation of 3D rotation representation using modified
///         rodrigues parametrisation without owning storage. This class
///         leverages Eigen::Map functionality to create a const Vector3 like
///         view of the underlying data array.
///
/// @tparam Scalar_     Floating type to use.
///
/// @tparam options_    Options to convey storage alignment.
///
template<typename Scalar_, int options_>
class Eigen::Map<const naksh::geometry::Rotation3<Scalar_>, options_> :
    public naksh::geometry::Mrp3<Map<const naksh::geometry::Rotation3<Scalar_>, options_>>
{
public:
    using Base = naksh::geometry::Mrp3<Map<const naksh::geometry::Rotation3<Scalar_>, options_>>;

    static constexpr auto kDimensions = Base::kDimensions;

    using Scalar = Scalar_;

    using Vector3 [[maybe_unused]] = Matrix<Scalar, kDimensions, 1>;

    using Matrix3 [[maybe_unused]] = Matrix<Scalar, kDimensions, kDimensions>;

    using Parameters = Map<const Vector3, options_>;


    /// @brief
    /// @param ptr
    explicit Map(const Scalar* ptr): mParameters(ptr) {}

    Map(const Map&) = default;

    Map(Map&&) noexcept = default;

    ~Map() = default;

    Map& operator=(const Map&) = default;

    Map& operator=(Map&&) noexcept = default;


    /// @brief
    /// @return
    inline const Parameters& cParameters() const noexcept { return mParameters; }


    /// @brief
    /// @return
    inline const Parameters& parameters() const noexcept { return cParameters(); }

private:
    Parameters mParameters;
};
