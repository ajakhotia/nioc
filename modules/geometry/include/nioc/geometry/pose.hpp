////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "traits.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iterator>
#include <nioc/common/exception.hpp>
#include <span>
#include <stdexcept>

namespace nioc::geometry
{
template<typename>
class Pose;

/// @brief 3D pose (SE3): position plus orientation.
///
/// Orientation is a unit quaternion; position is a 3D vector.
///
/// @tparam Derived The class deriving from this one (CRTP).
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

  /// @brief Const pointer to the 7 contiguous parameters [qx, qy, qz, qw, px, py, pz].
  [[nodiscard]] constexpr const Scalar* cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  /// @brief Same as @ref cData.
  [[nodiscard]] constexpr const Scalar* data() const noexcept
  {
    return cData();
  }

  /// @brief Mutable pointer to the 7 parameters.
  constexpr Scalar* data() noexcept
  {
    return derived().parameters().data();
  }

  /// @brief Read-only quaternion view of the orientation.
  [[nodiscard]] QuaternionConstMap cOrientation() const noexcept
  {
    return QuaternionConstMap(cData());
  }

  /// @brief Same as @ref cOrientation.
  [[nodiscard]] QuaternionConstMap orientation() const noexcept
  {
    return cOrientation();
  }

  /// @brief Writable quaternion view of the orientation.
  QuaternionMap orientation() noexcept
  {
    return QuaternionMap(data());
  }

  /// @brief Read-only vector view of the position.
  [[nodiscard]] Vector3ConstMap cPosition() const noexcept
  {
    return Vector3ConstMap(std::next(cData(), kNumOrientationParams));
  }

  /// @brief Same as @ref cPosition.
  [[nodiscard]] Vector3ConstMap position() const noexcept
  {
    return cPosition();
  }

  /// @brief Writable vector view of the position.
  Vector3Map position() noexcept
  {
    return Vector3Map(std::next(data(), kNumOrientationParams));
  }

  /// @brief Copy of this pose with its scalar type changed.
  /// @tparam ResultScalar Floating-point type of the returned pose.
  template<typename ResultScalar>
  [[nodiscard]] Pose<ResultScalar> cast() const
  {
    return Pose<ResultScalar>(std::span<const Scalar>(cData(), kNumParams));
  }

  /// @brief Inverse pose (a new copy).
  [[nodiscard]] Pose<Scalar> inverse() const
  {
    return Pose<Scalar>(derived()).invert();
  }

  /// @brief Inverts this pose in place.
  /// @return Reference to this pose.
  Derived& invert()
  {
    const Quaternion inverseOrientation = orientation().inverse();
    const Vector3 invertedPosition = Scalar(-1) * (inverseOrientation * position());
    orientation() = inverseOrientation;
    position() = invertedPosition;
    return derived();
  }

  /// @brief The identity pose (no rotation, zero position).
  static constexpr Pose<Scalar> identity()
  {
    return Pose<Scalar>({0, 0, 0, 1, 0, 0, 0});
  }

protected:
private:
  Se3() noexcept = default;

protected:
private:
  Se3(const Se3&) noexcept = default;

protected:
private:
  Se3(Se3&&) noexcept = default;

protected:
  ~Se3() noexcept = default;

  Se3& operator=(const Se3&) noexcept = default;

  Se3& operator=(Se3&&) noexcept = default;

private:
  [[nodiscard]] constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  [[nodiscard]] constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }

  friend Derived;
  friend Derived;
  friend Derived;
};

/// @brief 3D pose that owns its 7 parameters [qx, qy, qz, qw, px, py, pz].
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

  /// @brief Builds a pose from an orientation and a position.
  /// @param orientation Rotation quaternion. Normalized for you.
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

  /// @brief Builds a pose by copying a parameter array. Quaternion part is normalized.
  /// @param parameters 7 values [qx, qy, qz, qw, px, py, pz].
  explicit Pose(const Parameters& parameters) noexcept: mParameters(parameters)
  {
    Base::orientation().normalize();
  }

  /// @brief Builds a pose by moving a parameter array. Quaternion part is normalized.
  /// @param parameters 7 values [qx, qy, qz, qw, px, py, pz].
  explicit Pose(Parameters&& parameters) noexcept: mParameters(std::move(parameters))
  {
    Base::orientation().normalize();
  }

  /// @brief Builds a pose from a span of parameters. Quaternion part is normalized.
  /// @param span 7 values [qx, qy, qz, qw, px, py, pz].
  /// @throws std::invalid_argument if the span does not hold exactly 7 values.
  template<typename InputScalar>
  explicit Pose(const std::span<InputScalar>& span)
  {
    if(span.size() != kNumParams)
    {
      common::throwException<std::invalid_argument>(
          "Provided span is of insufficient size to initialize ({}) the underlying parameters. "
          "Required size is {}.",
          span.size(),
          kNumParams);
    }

    std::copy(span.begin(), span.end(), mParameters.begin());
    Base::orientation().normalize();
  }

  Pose(const Pose&) = default;

  Pose(Pose&&) noexcept = default;

  ~Pose() = default;

  Pose& operator=(const Pose&) = default;

  Pose& operator=(Pose&&) noexcept = default;

  /// @brief Read-only 7-element parameter array [qx, qy, qz, qw, px, py, pz].
  [[nodiscard]] constexpr const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Same as @ref cParameters.
  [[nodiscard]] constexpr const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Mutable parameter array.
  constexpr Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};

/// @brief Writes a pose to a stream.
/// @param stream Output stream.
/// @param se3 Pose to write.
/// @return The same stream.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Se3<Derived>& se3)
{
  static const auto kIoFormat =
      Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", "\n", "", "", "[", "]");


  stream << "{ Orientation:" << se3.cOrientation().coeffs().transpose().format(kIoFormat)
         << ", Position:" << se3.cPosition().transpose().format(kIoFormat) << " }";

  return stream;
}

/// @brief Composes two poses (lhs applied after rhs).
/// @param lhs Left pose.
/// @param rhs Right pose.
/// @return The composed pose.
template<typename LhsDerived, typename RhsDerived>
Pose<typename LhsDerived::Scalar> operator*(const Se3<LhsDerived>& lhs, const Se3<RhsDerived>& rhs)
{
  static_assert(
      std::is_same_v<typename LhsDerived::Scalar, typename RhsDerived::Scalar>,
      "Requested composition of Pose<> objects that differ in the scalar representation.");

  return {
      lhs.cOrientation() * rhs.cOrientation(),
      lhs.cPosition() + lhs.cOrientation() * rhs.cPosition()};
}

} // namespace nioc::geometry

/// @brief Writable pose view over external memory (no storage of its own).
///
/// Treats the buffer you pass as the pose parameters.
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

  /// @brief Maps a writable 7-element parameter buffer. Quaternion part is normalized.
  /// @param parameters View over 7 values [qx, qy, qz, qw, px, py, pz].
  /// @throws std::invalid_argument if the span does not hold exactly 7 values.
  explicit Map(const std::span<Scalar> parameters): mParameters(parameters)
  {
    if(parameters.size() != kNumParams)
    {
      nioc::common::throwException<std::invalid_argument>(
          "Provided span is of incorrect size ({}). Required size is {}.",
          parameters.size(),
          kNumParams);
    }
    Base::orientation().normalize();
  }

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Read-only view of the mapped 7 parameters.
  [[nodiscard]] constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Same as @ref cParameters.
  [[nodiscard]] constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Writable view of the mapped parameters.
  constexpr std::span<Scalar> parameters() noexcept
  {
    return mParameters;
  }

  /// @brief Copies the mapped parameters into an owned @ref Pose.
  explicit operator nioc::geometry::Pose<Scalar>() const
  {
    return nioc::geometry::Pose<Scalar>(cParameters());
  }

private:
  std::span<Scalar> mParameters;
};

/// @brief Read-only pose view over external memory (no storage of its own).
///
/// Treats the const buffer you pass as the pose parameters.
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

  /// @brief Maps a read-only 7-element parameter buffer. The quaternion part must already be unit
  /// norm; it is not normalized for you.
  /// @param parameters View over 7 values [qx, qy, qz, qw, px, py, pz].
  /// @throws std::invalid_argument if the span does not hold exactly 7 values, or the quaternion
  /// part is not unit norm.
  explicit Map(const std::span<const Scalar> parameters): mParameters(parameters)
  {
    if(parameters.size() != kNumParams)
    {
      nioc::common::throwException<std::invalid_argument>(
          "Provided span is of incorrect size ({}). Required size is {}.",
          parameters.size(),
          kNumParams);
    }

    if(std::abs(Base::cOrientation().norm() - Scalar(1)) > std::numeric_limits<Scalar>::epsilon())
    {
      nioc::common::throwException<std::invalid_argument>(
          "The quaternion portion of the passed parameters is not unit norm. Cannot proceed.");
    }
  }

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Read-only view of the mapped 7 parameters.
  [[nodiscard]] constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Same as @ref cParameters.
  [[nodiscard]] constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Copies the mapped parameters into an owned @ref Pose.
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
