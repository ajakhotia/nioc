////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace naksh::geometry
{


/// @brief
/// @tparam Derived
template<typename Derived>
class Se3
{
public:
    static constexpr auto kDimensions = 6U;

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

protected:

};


/// @brief
/// @tparam Derived
/// @param stream
/// @param se3
/// @return
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Se3<Derived>& se3)
{
    static const auto ioFormat = Eigen::IOFormat(
        Eigen::FullPrecision,
        0, ", ", "\n", "", "", "[", "]");

    const auto& derived = se3.cDerived();

    stream
        << "{ Orientation:" << derived.cOrientation().coeffs().transpose().format(ioFormat)
        << ", Position:" << derived.cPosition().transpose().format(ioFormat)
        << " }";

    return stream;
}


/// @brief
/// @tparam Scalar_
template<typename Scalar_>
class Pose : public Se3<Pose<Scalar_>>
{
public:
    using Base = Se3<Pose<Scalar_>>;

    static constexpr auto kDimensions = Base::kDimensions;

    static constexpr auto kNumParams = 7U;

    using Scalar = Scalar_;

    using Parameters = std::array<Scalar, kNumParams>;

    using Quaternion = Eigen::Quaternion<Scalar>;

    using Vector3 = Eigen::Vector3<Scalar>;

    using QuaternionMap = Eigen::Map<Quaternion>;

    using Vector3Map = Eigen::Map<Vector3>;


    Pose(const Quaternion& orientation, const Vector3& position):
        mParameters(makeParameterArray(orientation, position)),
        mOrientationMap(mParameters.data()),
        mPositionMap(mParameters.data() + 4U)
    {
    }

    Pose(const Pose&) = default;

    Pose(Pose&&) noexcept = default;

    ~Pose() = default;

    Pose& operator=(const Pose&) = default;

    Pose& operator=(Pose&&) noexcept = default;

    const QuaternionMap& cOrientation() const noexcept { return mOrientationMap; }

    const Vector3Map& cPosition() const noexcept { return mPositionMap; }

    const QuaternionMap& orientation() const noexcept { return cOrientation(); }

    const Vector3Map& position() const noexcept { return cPosition(); }

    QuaternionMap& orientation() noexcept { return mOrientationMap; }

    Vector3Map& position() noexcept { return mPositionMap; }


private:
    Parameters makeParameterArray(const Quaternion& orientation, const Vector3& position)
    {
        auto parameters = Parameters();
        std::memcpy(parameters.data(), orientation.coeffs().data(), sizeof(Scalar) * 4U);
        std::memcpy(parameters.data() + 4U, position.data(), sizeof(Scalar) * 3U);
        return parameters;
    }

    Parameters mParameters;

    QuaternionMap mOrientationMap;

    Vector3Map mPositionMap;
};

} // End of namespace naksh::geometry.


/// @brief
/// @tparam Scalar_
/// @tparam mapOptions
template<typename Scalar_, int mapOptions>
class Eigen::Map<naksh::geometry::Pose<Scalar_>, mapOptions> :
    public naksh::geometry::Se3<Eigen::Map<naksh::geometry::Pose<Scalar_>, mapOptions>>
{
public:
    using Base = naksh::geometry::Se3<Eigen::Map<naksh::geometry::Pose<Scalar_>, mapOptions>>;

    static constexpr auto kDimensions = Base::kDimensions;

    using Scalar = Scalar_;

    using Quaternion = Eigen::Quaternion<Scalar>;

    using Vector3 = Eigen::Vector3<Scalar>;

    using QuaternionMap = Eigen::Map<Quaternion>;

    using Vector3Map = Eigen::Map<Vector3>;


    explicit Map(Scalar* storageArrayPtr):
        mOrientationMap(storageArrayPtr),
        mPositionMap(storageArrayPtr + 4U)
    {
    }

    Map(const Map&) = default;

    Map(Map&&) noexcept = default;

    ~Map() = default;

    Map& operator=(const Map&) = default;

    Map& operator=(Map&&) noexcept = default;

    const QuaternionMap& cOrientation() const noexcept { return mOrientationMap; }

    const Vector3Map& cPosition() const noexcept { return mPositionMap; }

    const QuaternionMap& orientation() const noexcept { return cOrientation(); }

    const Vector3Map& position() const noexcept { return cPosition(); }

    QuaternionMap& orientation() noexcept { return mOrientationMap; }

    Vector3Map& position() noexcept { return mPositionMap; }

private:
    QuaternionMap mOrientationMap;

    Vector3Map mPositionMap;
};


/// @brief
/// @tparam Scalar_
/// @tparam mapOptions
template<typename Scalar_, int mapOptions>
class Eigen::Map<const naksh::geometry::Pose<Scalar_>, mapOptions> :
    public naksh::geometry::Se3<Eigen::Map<const naksh::geometry::Pose<Scalar_>, mapOptions>>
{
public:
    using Base = naksh::geometry::Se3<Eigen::Map<const naksh::geometry::Pose<Scalar_>, mapOptions>>;

    static constexpr auto kDimensions = Base::kDimensions;

    using Scalar = Scalar_;

    using Quaternion = Eigen::Quaternion<Scalar>;

    using Vector3 = Eigen::Vector3<Scalar>;

    using QuaternionMap = Eigen::Map<const Quaternion>;

    using Vector3Map = Eigen::Map<const Vector3>;

    explicit Map(const Scalar* storageArrayPtr):
        mOrientationMap(storageArrayPtr),
        mPositionMap(storageArrayPtr + 4U)
    {
    }

    Map(const Map&) = default;

    Map(Map&&) noexcept = default;

    ~Map() = default;

    Map& operator=(const Map&) = default;

    Map& operator=(Map&&) noexcept = default;

    const QuaternionMap& cOrientation() const noexcept { return mOrientationMap; }

    const Vector3Map& cPosition() const noexcept { return mPositionMap; }

    const QuaternionMap& orientation() const noexcept { return cOrientation(); }

    const Vector3Map& position() const noexcept { return cPosition(); }

private:
    QuaternionMap mOrientationMap;

    Vector3Map mPositionMap;
};
