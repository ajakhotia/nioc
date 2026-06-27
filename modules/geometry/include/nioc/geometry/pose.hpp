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

/// @brief The shared rigid-body transform (SE(3)) interface that every pose representation
/// inherits.
///
/// A pose maps a point from a child frame into a parent frame by rotating then translating it:
/// `p_parent = orientation * p_child + position`. This is a CRTP base, so it is never used on its
/// own; instantiate `Pose` (owns its data) or an `Eigen::Map<Pose>` (views data owned elsewhere)
/// and call this interface through them.
///
/// The seven scalar parameters live in `Derived` and are reached through `cParameters()` /
/// `parameters()`. Their layout is fixed: indices 0-3 are the orientation quaternion in Eigen
/// coefficient order (x, y, z, w), indices 4-6 are the position (x, y, z). The quaternion is
/// assumed to be unit-norm; constructors of the concrete types enforce that, but writing through
/// the mutable accessors here does not.
///
/// @tparam Derived The concrete pose type. Its `Scalar` comes from `Traits<Derived>`.
///
/// @see Pose, Traits
template<typename Derived>
class Se3
{
public:
  /// Total number of stored scalars: four for orientation plus three for position.
  static constexpr auto kNumParams = 7U;

  /// Number of leading scalars holding the orientation quaternion.
  static constexpr auto kNumOrientationParams = 4U;

  /// Number of trailing scalars holding the position.
  static constexpr auto kNumPositionParams = 3U;

  using Scalar = Traits<Derived>::Scalar;

  using Quaternion = Eigen::Quaternion<Scalar>;

  using Vector3 = Eigen::Vector3<Scalar>;

  using QuaternionMap = Eigen::Map<Quaternion>;

  using Vector3Map = Eigen::Map<Vector3>;

  using QuaternionConstMap = Eigen::Map<const Quaternion>;

  using Vector3ConstMap = Eigen::Map<const Vector3>;

  /// @brief Return a read-only pointer to the seven contiguous parameters, orientation first.
  ///
  /// The pointer aliases `Derived`'s parameter storage. For a map that storage is the caller's
  /// buffer, so the pointer stays valid as long as that buffer lives, not merely this object.
  [[nodiscard]] constexpr const Scalar* cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  /// Const overload of `data()`; aliases `cData()`.
  [[nodiscard]] constexpr const Scalar* data() const noexcept
  {
    return cData();
  }

  /// @brief Return a mutable pointer to the seven contiguous parameters, orientation first.
  ///
  /// Writing through it bypasses normalization, so keep the orientation portion unit-norm
  /// yourself. Lifetime matches `cData()`.
  constexpr Scalar* data() noexcept
  {
    return derived().parameters().data();
  }

  /// @brief View the orientation as a read-only quaternion mapped onto the stored parameters.
  ///
  /// The returned map aliases the parameter storage; it is valid only while that storage lives and
  /// stays in place.
  [[nodiscard]] QuaternionConstMap cOrientation() const noexcept
  {
    return QuaternionConstMap(cData());
  }

  /// Const overload of `orientation()`; aliases `cOrientation()`.
  [[nodiscard]] QuaternionConstMap orientation() const noexcept
  {
    return cOrientation();
  }

  /// @brief View the orientation as a mutable quaternion mapped onto the stored parameters.
  ///
  /// Assigning through the map writes the four orientation scalars in place and does not
  /// renormalize; supply a unit quaternion. Valid only while the parameter storage lives and stays
  /// in place.
  QuaternionMap orientation() noexcept
  {
    return QuaternionMap(data());
  }

  /// @brief View the position as a read-only 3-vector mapped onto the stored parameters.
  ///
  /// The returned map aliases the parameter storage; it is valid only while that storage lives and
  /// stays in place.
  [[nodiscard]] Vector3ConstMap cPosition() const noexcept
  {
    return Vector3ConstMap(std::next(cData(), kNumOrientationParams));
  }

  /// Const overload of `position()`; aliases `cPosition()`.
  [[nodiscard]] Vector3ConstMap position() const noexcept
  {
    return cPosition();
  }

  /// @brief View the position as a mutable 3-vector mapped onto the stored parameters.
  ///
  /// The returned map aliases the parameter storage; it is valid only while that storage lives and
  /// stays in place.
  Vector3Map position() noexcept
  {
    return Vector3Map(std::next(data(), kNumOrientationParams));
  }

  /// @brief Return an owning copy of this pose with its scalars converted to `ResultScalar`.
  ///
  /// Example:
  ///
  ///     Pose<double> p = ...;
  ///     Pose<float> f = p.cast<float>();
  ///
  /// The result renormalizes its orientation.
  ///
  /// @tparam ResultScalar The scalar type of the copy. Must be named explicitly.
  template<typename ResultScalar>
  [[nodiscard]] Pose<ResultScalar> cast() const
  {
    return Pose<ResultScalar>(std::span<const Scalar>(cData(), kNumParams));
  }

  /// @brief Return the inverse transform as a fresh owning `Pose`, leaving this object unchanged.
  [[nodiscard]] Pose<Scalar> inverse() const
  {
    return Pose<Scalar>(derived()).invert();
  }

  /// @brief Invert this transform in place and return a reference to the derived object.
  Derived& invert()
  {
    const Quaternion inverseOrientation = orientation().inverse();
    const Vector3 invertedPosition = Scalar(-1) * (inverseOrientation * position());
    orientation() = inverseOrientation;
    position() = invertedPosition;
    return derived();
  }

  /// @brief Return the identity pose: no rotation, zero translation.
  static constexpr Pose<Scalar> identity()
  {
    return Pose<Scalar>({0, 0, 0, 1, 0, 0, 0});
  }

private:
  Se3() noexcept = default;

  Se3(const Se3&) noexcept = default;

  Se3(Se3&&) noexcept = default;

  ~Se3() noexcept = default;

  Se3& operator=(const Se3&) noexcept = default;

  Se3& operator=(Se3&&) noexcept = default;

  /// @brief Grant the concrete pose access to the otherwise-private special members.
  ///
  /// CRTP needs `Derived` to construct, copy, move, and destroy this base. Keeping these members
  /// private and befriending only `Derived` gives it that access while preventing `Se3` from being
  /// inherited as an ordinary base by an unrelated class (the misuse `bugprone-crtp-constructor-`
  /// `accessibility` guards against).
  friend Derived;

  /// @brief Downcast this base to a read-only reference to the concrete `Derived` pose.
  ///
  /// The CRTP contract guarantees that every `Se3` is in fact a `Derived`, so the static cast is
  /// always sound. Used by the interface methods to reach `Derived`'s parameter storage.
  [[nodiscard]] constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  /// Const overload of `derived()`; aliases `cDerived()`.
  [[nodiscard]] constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  /// @brief Downcast this base to a mutable reference to the concrete `Derived` pose.
  ///
  /// Same CRTP soundness guarantee as `cDerived()`. Used by the mutating interface methods.
  constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }
};

/// @brief A value-semantic rigid-body transform (SE(3)) that owns its seven parameters inline.
///
/// The concrete pose-object to reach for when you need to store or pass a transform by value. It
/// holds a `std::array` of orientation (quaternion) then position, and every constructor
/// renormalizes the quaternion, so a `Pose` always represents a valid rotation regardless of input
/// precision. To operate on parameters owned elsewhere, use `Eigen::Map<Pose>` instead. Composition
/// (`operator*`) and the inherited operations yield and accept `Pose`.
///
/// @tparam Scalar_ The floating-point scalar of each parameter.
///
/// @see Se3, operator*
template<typename Scalar_>
class Pose: public Se3<Pose<Scalar_>>
{
public:
  using Base = Se3<Pose<Scalar_>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = Base::Scalar;

  using Quaternion = Base::Quaternion;

  using Vector3 = Base::Vector3;

  /// Inline storage for the seven parameters: orientation (x, y, z, w), then position (x, y, z).
  using Parameters = std::array<Scalar, kNumParams>;

  /// @brief Construct from an explicit orientation and position; the orientation is normalized.
  Pose(const Quaternion& orientation, const Vector3& position)
  {
    std::memcpy(
        parameters().data(),
        orientation.coeffs().data(),
        sizeof(Scalar) * orientation.coeffs().size());

    std::memcpy(
        std::next(parameters().data(), orientation.coeffs().size()),
        position.data(),
        sizeof(Scalar) * position.size());

    Base::orientation().normalize();
  }

  /// @brief Construct from a raw 7-parameter array, orientation first; the orientation is
  /// normalized.
  explicit Pose(const Parameters& parameters) noexcept: mParameters(parameters)
  {
    Base::orientation().normalize();
  }

  /// @brief Construct by moving a raw 7-parameter array; the orientation is normalized.
  explicit Pose(Parameters&& parameters) noexcept: mParameters(std::move(parameters))
  {
    Base::orientation().normalize();
  }

  /// @brief Construct by copying seven parameters out of a span; the orientation is normalized.
  ///
  /// The span is only read from; no reference to it is retained.
  ///
  /// @tparam InputScalar The span's element-type. Maybe `const`.
  ///
  /// @param span Must have exactly `kNumParams` elements.
  ///
  /// @throws std::invalid_argument If `span.size() != kNumParams`.
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

  /// @brief Return the seven parameters by const reference: orientation first, then position.
  [[nodiscard]] constexpr const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// Const overload of `parameters()`; aliases `cParameters()`.
  [[nodiscard]] constexpr const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Return the seven parameters by mutable reference.
  ///
  /// Editing them directly bypasses normalization; keep the orientation portion unit-norm.
  constexpr Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};

/// @brief Stream the pose as `{ Orientation:[x, y, z, w], Position:[x, y, z] }`.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Se3<Derived>& se3)
{
  static const auto kIoFormat =
      Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", "\n", "", "", "[", "]");


  stream << "{ Orientation:" << se3.cOrientation().coeffs().transpose().format(kIoFormat)
         << ", Position:" << se3.cPosition().transpose().format(kIoFormat) << " }";

  return stream;
}

/// @brief Compose two transforms: apply `rhs` then `lhs`, returning the combined pose.
///
/// Mirrors matrix multiplication of homogeneous transforms, so the order matters: `a * b` is not
/// `b * a`. The result is a fresh owning `Pose`.
///
/// @return A `Pose` with the left operand's `Scalar`.
///
/// @note Both operands must share the same `Scalar`; a mismatch fails a static assertion.
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

/// @brief A non-owning, mutable SE(3) view over seven pose parameters owned elsewhere.
///
/// Stores an `std::span<Scalar>` over caller-supplied storage and exposes the full `Se3`
/// interface against it. Reads and writes go straight through to that buffer, so changes are seen
/// by whoever owns it. Convert to an owning `Pose` with an explicit cast.
///
/// @tparam Scalar_ The floating-point scalar of each parameter.
///
/// @note The referenced storage must outlive the map and stay fixed in place.
///
/// @see nioc::geometry::Pose, Eigen::Map<const nioc::geometry::Pose>
template<typename Scalar_>
class Eigen::Map<nioc::geometry::Pose<Scalar_>>:
  public nioc::geometry::Se3<Eigen::Map<nioc::geometry::Pose<Scalar_>>>
{
public:
  using Base = nioc::geometry::Se3<Eigen::Map<nioc::geometry::Pose<Scalar_>>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = Base::Scalar;

  /// @brief Bind the view to `parameters` and normalize the orientation in that storage.
  ///
  /// Retains a reference to the buffer, which must outlive this map; the normalization mutates that
  /// buffer in place.
  ///
  /// @param parameters Must have exactly `kNumParams` elements.
  ///
  /// @throws std::invalid_argument If `parameters.size() != kNumParams`.
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

  /// @brief Return a read-only span over the viewed parameters.
  [[nodiscard]] constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  /// Const overload of `parameters()`; aliases `cParameters()`.
  [[nodiscard]] constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Return a mutable span over the viewed parameters; writes reach the underlying storage.
  constexpr std::span<Scalar> parameters() noexcept
  {
    return mParameters;
  }

  /// @brief Copy the viewed parameters into a fresh owning `Pose`.
  explicit operator nioc::geometry::Pose<Scalar>() const
  {
    return nioc::geometry::Pose<Scalar>(cParameters());
  }

private:
  std::span<Scalar> mParameters;
};

/// @brief A non-owning, read-only SE(3) view over seven pose parameters owned elsewhere.
///
/// The const counterpart of `Eigen::Map<Pose>`: a view over `std::span<const Scalar>` exposing
/// only the read side of the `Se3` interface. Because it cannot mutate its storage, it never
/// renormalizes; instead, the constructor demands an already unit-norm orientation. Convert to an
/// owning `Pose` with an explicit cast.
///
/// @tparam Scalar_ The floating-point scalar of each parameter.
///
/// @note The referenced storage must outlive the map and stay fixed in place.
///
/// @see nioc::geometry::Pose, Eigen::Map<nioc::geometry::Pose>
template<typename Scalar_>
class Eigen::Map<const nioc::geometry::Pose<Scalar_>>:
  public nioc::geometry::Se3<Eigen::Map<const nioc::geometry::Pose<Scalar_>>>
{
public:
  using Base = nioc::geometry::Se3<Eigen::Map<const nioc::geometry::Pose<Scalar_>>>;

  static constexpr auto kNumParams = Base::kNumParams;

  using Scalar = Base::Scalar;

  /// @brief Bind the read-only view to `parameters`, requiring an already unit-norm orientation.
  ///
  /// Retains a reference to the buffer, which must outlive this map.
  ///
  /// @param parameters Must have exactly `kNumParams` elements with a unit-norm orientation
  /// quaternion.
  ///
  /// @throws std::invalid_argument If `parameters.size() != kNumParams`, or if the orientation
  /// norm differs from 1 by more than machine epsilon.
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

  /// @brief Return a read-only span over the viewed parameters.
  [[nodiscard]] constexpr std::span<const Scalar> cParameters() const noexcept
  {
    return mParameters;
  }

  /// Const overload of `parameters()`; aliases `cParameters()`.
  [[nodiscard]] constexpr std::span<const Scalar> parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Copy the viewed parameters into a fresh owning `Pose`.
  explicit operator nioc::geometry::Pose<Scalar>() const
  {
    return nioc::geometry::Pose<Scalar>(cParameters());
  }

private:
  std::span<const Scalar> mParameters;
};

namespace nioc::geometry
{
/// @brief `Traits` specialization exposing `Pose`'s scalar type to the `Se3` CRTP base.
template<typename Scalar_>
struct Traits<Pose<Scalar_>>
{
  using Scalar = Scalar_;
};

/// @brief `Traits` specialization exposing a mutable `Pose` map's scalar type to the `Se3` CRTP
/// base.
template<typename Scalar_>
struct Traits<Eigen::Map<Pose<Scalar_>>>
{
  using Scalar = Scalar_;
};

/// @brief `Traits` specialization exposing a read-only `Pose` map's scalar type to the `Se3` CRTP
/// base.
template<typename Scalar_>
struct Traits<Eigen::Map<const Pose<Scalar_>>>
{
  using Scalar = Scalar_;
};


} // namespace nioc::geometry
