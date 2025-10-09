////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "traits.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <span>

namespace nioc::geometry
{
/// @brief Forward declaration of the pose class.
template<typename>
class Pose;

/// @brief 3D pose representation (SE3 group).
///
/// Combines position and orientation in 3D space. Orientation uses unit quaternions, position uses 3D vectors.
///
/// @tparam Derived CRTP derived type.
template<typename Derived>
class Se3
{
public:
  static constexpr auto kNumParams = 7U;

  static constexpr auto kNumOrientationParams = 4U;

  static constexpr auto kNumPositionParams = 3U;

  using Scalar = typename Traits<Derived>::Scalar;

  using Quaternion = Eigen::Quaternion<Scalar>;

  using Vector3 = Eigen::Vector3<Scalar>;

  using QuaternionMap = Eigen::Map<Quaternion>;

  using Vector3Map = Eigen::Map<Vector3>;

  using QuaternionConstMap = Eigen::Map<const Quaternion>;

  using Vector3ConstMap = Eigen::Map<const Vector3>;

  constexpr const Scalar* cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  constexpr const Scalar* data() const noexcept
  {
    return cData();
  }

  constexpr Scalar* data() noexcept
  {
    return derived().parameters().data();
  }

  QuaternionConstMap cOrientation() const noexcept
  {
    return QuaternionConstMap(cData());
  }

  QuaternionConstMap orientation() const noexcept
  {
    return cOrientation();
  }

  QuaternionMap orientation() noexcept
  {
    return QuaternionMap(data());
  }

  Vector3ConstMap cPosition() const noexcept
  {
    return Vector3ConstMap(cData() + kNumOrientationParams);
  }

  Vector3ConstMap position() const noexcept
  {
    return cPosition();
  }

  Vector3Map position() noexcept
  {
    return Vector3Map(data() + kNumOrientationParams);
  }

  template<typename ResultScalar>
  Pose<ResultScalar> cast() const noexcept
  {
    return Pose<ResultScalar>(std::span<const Scalar>(cData(), kNumParams));
  }

  /// @brief Computes inverse pose.
  /// @return Inverted pose.
  Pose<Scalar> inverse() const
  {
    return Pose<Scalar>(derived()).invert();
  }

  /// @brief Inverts pose in-place.
  /// @return Reference to this.
  Derived& invert()
  {
    const auto inverseOrientation = orientation().inverse();
    const auto negativeInversePosition = inverseOrientation * position();
    *this = Pose<Scalar>(inverseOrientation, Scalar(-1) * negativeInversePosition);
    return derived();
  }

  /// @brief Creates identity pose.
  /// @return Identity transformation.
  static constexpr Pose<Scalar> identity()
  {
    return Pose<Scalar>({ 0, 0, 0, 1, 0, 0, 0 });
  }

protected:
  Se3() noexcept = default;

  Se3(const Se3&) noexcept = default;

  Se3(Se3&&) noexcept = default;

  ~Se3() noexcept = default;

  Se3& operator=(const Se3&) noexcept = default;

  Se3& operator=(Se3&&) noexcept = default;

private:
  inline constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  inline constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  inline constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }
};

/// @brief 3D pose with owned storage.
///
/// Stores 7 parameters: [qx, qy, qz, qw, px, py, pz] (quaternion + position).
///
/// @tparam Scalar_ Floating-point type (float, double).
template<typename Scalar_>
class Pose: public Se3<Pose<Scalar_>>
{
public:
  using Base = Se3<Pose<Scalar_>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = typename Base::Scalar;

  using Quaternion = typename Base::Quaternion;

  using Vector3 = typename Base::Vector3;

  using Parameters = std::array<Scalar, kNumParams>;

  /// @brief Constructs from orientation and position.
  /// @param orientation Rotation quaternion (normalized automatically).
  /// @param position Position vector.
  Pose(const Quaternion& orientation, const Vector3& position)
  {
    std::memcpy(
        parameters().data(),
        orientation.coeffs().data(),
        sizeof(Scalar) * orientation.coeffs().size());

    std::memcpy(
        parameters().data() + orientation.coeffs().size(),
        position.data(),
        sizeof(Scalar) * position.size());

    Base::orientation().normalize();
  }

  /// @brief Constructs from parameter array (copy).
  /// @param parameters Array of 7 values [qx, qy, qz, qw, px, py, pz].
  explicit Pose(const Parameters& parameters) noexcept: mParameters(parameters)
  {
    Base::orientation().normalize();
  }

  /// @brief Constructs from parameter array (move).
  /// @param parameters Array of 7 values [qx, qy, qz, qw, px, py, pz].
  explicit Pose(Parameters&& parameters) noexcept: mParameters(std::move(parameters))
  {
    Base::orientation().normalize();
  }

  /// @brief Constructs from span of parameters.
  /// @param span Span containing 7 values.
  /// @throws std::invalid_argument if span size is not 7.
  template<typename InputScalar>
  explicit Pose(const std::span<InputScalar>& span)
  {
    if(span.size() != kNumParams)
    {
      throw std::invalid_argument(
          "[geometry::Pose] Provided span is of insufficient size to initialize (" +
          std::to_string(span.size()) + ") the underlying parameters. Required size is " +
          std::to_string(kNumParams) + ".");
    }

    std::copy(span.begin(), span.end(), mParameters.begin());
    Base::orientation().normalize();
  }

  Pose(const Pose&) = default;

  Pose(Pose&&) noexcept = default;

  ~Pose() = default;

  Pose& operator=(const Pose&) = default;

  Pose& operator=(Pose&&) noexcept = default;

  constexpr const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  constexpr const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  constexpr Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};

/// @brief Outputs pose to stream.
/// @param stream Output stream.
/// @param se3 Pose to output.
/// @return Modified stream.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Se3<Derived>& se3)
{
  static const auto ioFormat =
      Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", "\n", "", "", "[", "]");


  stream << "{ Orientation:" << se3.cOrientation().coeffs().transpose().format(ioFormat)
         << ", Position:" << se3.cPosition().transpose().format(ioFormat) << " }";

  return stream;
}

/// @brief Composes two poses.
/// @param lhs First pose.
/// @param rhs Second pose.
/// @return Composed pose.
template<typename LhsDerived, typename RhsDerived>
Pose<typename LhsDerived::Scalar> operator*(const Se3<LhsDerived>& lhs, const Se3<RhsDerived>& rhs)
{
  static_assert(
      std::is_same_v<typename LhsDerived::Scalar, typename RhsDerived::Scalar>,
      "Requested composition of Pose<> objects that differ in the scalar representation.");

  return { lhs.cOrientation() * rhs.cOrientation(),
           lhs.cPosition() + lhs.cOrientation() * rhs.cPosition() };
}

} // End of namespace nioc::geometry.

/// @brief Pose without owned storage (mutable).
///
/// Maps existing memory as pose parameters.
///
/// @tparam Scalar_ Floating-point type.
template<typename Scalar_>
class Eigen::Map<nioc::geometry::Pose<Scalar_>>:
    public nioc::geometry::Se3<Eigen::Map<nioc::geometry::Pose<Scalar_>>>
{
public:
  using Base = nioc::geometry::Se3<Eigen::Map<nioc::geometry::Pose<Scalar_>>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = typename Base::Scalar;

  explicit Map(const std::span<Scalar> parameters): mParameters(parameters)
  {
    if(parameters.size() != kNumParams)
    {
      throw std::invalid_argument(
          "[Eigen::Map<geometry::Pose>] Provided span is "
          "of incorrect size(" +
          std::to_string(parameters.size()) +
          "). Required "
          "size is " +
          std::to_string(kNumParams) + ".");
    }
    Base::orientation().normalize();
  }

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  constexpr std::span<Scalar> parameters() noexcept
  {
    return mParameters;
  }

  explicit operator nioc::geometry::Pose<Scalar>() const
  {
    return nioc::geometry::Pose<Scalar>(cParameters());
  }

private:
  std::span<Scalar> mParameters;
};

/// @brief Pose without owned storage (const).
///
/// Maps const memory as pose parameters.
///
/// @tparam Scalar_ Floating-point type.
template<typename Scalar_>
class Eigen::Map<const nioc::geometry::Pose<Scalar_>>:
    public nioc::geometry::Se3<Eigen::Map<const nioc::geometry::Pose<Scalar_>>>
{
public:
  using Base = nioc::geometry::Se3<Eigen::Map<const nioc::geometry::Pose<Scalar_>>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = typename Base::Scalar;

  explicit Map(const std::span<const Scalar> parameters): mParameters(parameters)
  {
    if(parameters.size() != kNumParams)
    {
      throw std::invalid_argument(
          "[Eigen::Map<const geometry::Pose>] Provided span is "
          "of incorrect size(" +
          std::to_string(parameters.size()) +
          "). Required "
          "size is " +
          std::to_string(kNumParams) + ".");
    }

    if(std::abs(Base::cOrientation().norm() - Scalar(1)) > std::numeric_limits<Scalar>::epsilon())
    {
      throw std::invalid_argument(
          "[Eigen::Map<const geometry::Pose>] The quaternion "
          "portion of the passed parameters is not unit norm. Cannot proceed.");
    }
  }

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  explicit operator nioc::geometry::Pose<Scalar>() const
  {
    return nioc::geometry::Pose<Scalar>(cParameters());
  }

private:
  std::span<const Scalar> mParameters;
};

namespace nioc::geometry
{
/// @brief Traits for Pose.
template<typename Scalar_>
struct Traits<Pose<Scalar_>>
{
  using Scalar = Scalar_;
};

/// @brief Traits for mapped Pose.
template<typename Scalar_>
struct Traits<Eigen::Map<Pose<Scalar_>>>
{
  using Scalar = Scalar_;
};

/// @brief Traits for const mapped Pose.
template<typename Scalar_>
struct Traits<Eigen::Map<const Pose<Scalar_>>>
{
  using Scalar = Scalar_;
};


} // namespace nioc::geometry
