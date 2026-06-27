////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <utility>

namespace nioc::geometry
{
/// @brief An owning 3D rotation stored as Modified Rodrigues Parameters (MRP).
///
/// @tparam Scalar_ The coordinate type; defaults to `double`.
///
/// @see Rotation3 (definition below) for the full interface.
template<typename Scalar_ = double>
class Rotation3;

} // namespace nioc::geometry


/// @brief A non-owning, mutable view of a `Rotation3`'s three MRP coordinates in caller-owned
/// memory.
///
/// @see The specialization below for the full interface.
template<typename Scalar_, int MapOptions>
class Eigen::Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>;


/// @brief A non-owning, read-only view of a `Rotation3`'s three MRP coordinates in caller-owned
/// memory.
///
/// @see The specialization below for the full interface.
template<typename Scalar_, int MapOptions>
class Eigen::Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>;

namespace nioc::geometry
{
/// @brief A CRTP base that supplies the shared rotation algebra for any Modified Rodrigues
/// Parameter (MRP) representation: coordinate access, angle/axis extraction, inverse, composition,
/// and streaming.
///
/// The MRP 3-vector `p = tan(theta/4) * axis` encodes a rotation of angle `theta` about unit
/// `axis`. The zero vector is identity, and `|p| < 1` covers rotations up to half a turn. This base
/// holds no state of its own; the coordinates live in `Derived`.
///
/// @tparam Derived The concrete representation. It must expose a `Scalar` type and a const
/// `cParameters()` returning the MRP 3-vector; the mutating accessors additionally
/// require a `parameters()` returning a writable 3-vector.
///
/// Inherit from this base; do not instantiate it directly. Its constructors are private and
/// befriended to `Derived`.
///
/// @see Rotation3, Eigen::Map<Rotation3>, Eigen::Map<const Rotation3>
template<typename Derived>
class Mrp3
{
public:
  /// The number of MRP coordinates; always 3.
  static constexpr auto kDimensions = 3U;

  /// @brief Return a const reference to `*this` viewed as the concrete `Derived` object.
  [[nodiscard]] constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  /// @brief Return a const reference to `*this` viewed as the concrete `Derived` object.
  ///
  /// @see cDerived, the non-const overload that returns a writable reference.
  [[nodiscard]] constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  /// @brief Return a writable reference to `*this` viewed as the concrete `Derived` object.
  constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }

  /// @brief Return a read-only pointer to the first of the three contiguous MRP coordinates.
  [[nodiscard]] decltype(auto) cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  /// @brief Return a read-only pointer to the first of the three contiguous MRP coordinates.
  ///
  /// @see cData, data (the non-const overload returns a writable pointer).
  [[nodiscard]] decltype(auto) data() const noexcept
  {
    return cData();
  }

  /// @brief Return a writable pointer to the first of the three contiguous MRP coordinates.
  ///
  /// Writes go straight to the coordinate storage and are not validated.
  decltype(auto) data() noexcept
  {
    return derived().parameters().data();
  }

  /// @brief Return the first MRP coordinate.
  [[nodiscard]] decltype(auto) x() const noexcept
  {
    return cDerived().cParameters().x();
  }

  /// @brief Return the second MRP coordinate.
  [[nodiscard]] decltype(auto) y() const noexcept
  {
    return cDerived().cParameters().y();
  }

  /// @brief Return the third MRP coordinate.
  [[nodiscard]] decltype(auto) z() const noexcept
  {
    return cDerived().cParameters().z();
  }

  /// @brief Return a writable reference to the first MRP coordinate; writes are not validated.
  decltype(auto) x() noexcept
  {
    return derived().parameters().x();
  }

  /// @brief Return a writable reference to the second MRP coordinate; writes are not validated.
  decltype(auto) y() noexcept
  {
    return derived().parameters().y();
  }

  /// @brief Return a writable reference to the third MRP coordinate; writes are not validated.
  decltype(auto) z() noexcept
  {
    return derived().parameters().z();
  }

  /// @brief Return the rotation angle in radians, in the range `[0, 2*pi)`.
  [[nodiscard]] decltype(auto) angle() const noexcept
  {
    using Scalar = Derived::Scalar;
    return Scalar(4) * std::atan(cDerived().cParameters().norm());
  }

  /// @brief Return the unit rotation axis.
  ///
  /// Undefined for the identity rotation: normalizing the zero MRP vector yields a zero vector.
  [[nodiscard]] auto axis() const noexcept
  {
    return cDerived().cParameters().normalized().eval();
  }

  /// @brief Return the inverse rotation as a new owning `Rotation3`, regardless of `Derived`'s
  /// storage.
  [[nodiscard]] decltype(auto) inverse() const
  {
    return Rotation3<typename Derived::Scalar>(
        typename Derived::Scalar(-1) * cDerived().cParameters());
  }

  /// @brief Compose two rotations and return the result as a new owning `Rotation3`.
  ///
  /// The result applies @p rhsBase first, then `*this`, matching rotation-matrix multiplication.
  decltype(auto) operator*(const Mrp3& rhsBase) const
  {
    using Scalar = Derived::Scalar;

    const auto& lhsMrp = this->cDerived().cParameters();
    const auto& rhsMrp = rhsBase.cDerived().cParameters();

    const auto lhsNorm2 = lhsMrp.squaredNorm();
    const auto rhsNorm2 = rhsMrp.squaredNorm();

    const auto vec = (((Scalar(1) - rhsNorm2) * lhsMrp) +
                      ((Scalar(1) - lhsNorm2) * rhsMrp) +
                      (Scalar(2) * lhsMrp.cross(rhsMrp)))
                         .eval();

    const auto scale = Scalar(1) + (lhsNorm2 * rhsNorm2) - (Scalar(2) * lhsMrp.dot(rhsMrp));

    return Rotation3<Scalar>(vec / scale);
  }

private:
  friend Derived;

  Mrp3() = default;

  Mrp3(const Mrp3&) noexcept = default;

  Mrp3(Mrp3&&) noexcept = default;

protected:
  ~Mrp3() = default;

  Mrp3& operator=(const Mrp3&) noexcept = default;

  Mrp3& operator=(Mrp3&&) noexcept = default;
};

/// @brief Write the MRP coordinates to @p stream as a full-precision row vector `[x, y, z]`.
///
/// @return @p stream, to allow chaining.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Mrp3<Derived>& mrp3)
{
  static const auto kIoFormat =
      Eigen::IOFormat(Eigen::FullPrecision, 0, ", ", "\n", "", "", "[", "]");

  stream << mrp3.cDerived().cParameters().transpose().format(kIoFormat);
  return stream;
}

/// @brief A 3D rotation that owns its Modified Rodrigues Parameter coordinate vector.
///
/// Example:
///
///     // 90 degrees about the Z axis, three equivalent ways.
///     auto r0 = Rotation3<double>(M_PI / 2, Eigen::Vector3d::UnitZ());
///     auto r1 = Rotation3<double>(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
///     auto r2 = r0 * r1;            // compose: apply r1 first, then r0.
///     double theta = r2.angle();   // extract the rotation angle.
///
/// The MRP vector `p = tan(theta/4) * axis` is stored inline. All rotation algebra (composition,
/// inverse, angle/axis extraction, streaming) is inherited from `Mrp3`.
///
/// @tparam Scalar_ The coordinate type; defaults to `double`.
///
/// Construct from raw MRP coordinates, an angle/axis pair, an `Eigen::AngleAxis`, or an
/// `Eigen::Quaternion`. Concurrent mutation through `parameters()` is not synchronized.
///
/// @see Mrp3, Eigen::Map<Rotation3>, Eigen::Map<const Rotation3>
template<typename Scalar_>
class Rotation3: public Mrp3<Rotation3<Scalar_>>
{
public:
  using Base = Mrp3<Rotation3<Scalar_>>;

  static constexpr auto kDimensions = Base::kDimensions;

  using Scalar = Scalar_;

  using Vector3 = Eigen::Matrix<Scalar, kDimensions, 1>;

  using Matrix3 = Eigen::Matrix<Scalar, kDimensions, kDimensions>;

  using Quaternion = Eigen::Quaternion<Scalar>;

  using AngleAxis = Eigen::AngleAxis<Scalar>;

  /// The owning storage type for the MRP coordinates: a plain 3-vector.
  using Parameters = Vector3;

  /// @brief Adopt @p parameters verbatim as the MRP coordinates.
  ///
  /// @param parameters The MRP 3-vector. Taken as-is; no normalization or validation.
  explicit Rotation3(Vector3 parameters): mParameters(std::move(parameters)) {}

  /// @brief Construct a rotation of @p angle radians about @p axis.
  ///
  /// @param angle The rotation angle in radians.
  ///
  /// @param axis The rotation axis. Need not be unit length; it is normalized internally.
  Rotation3(const Scalar angle, const Vector3& axis):
    Rotation3(axis.normalized() * std::tan(angle / Scalar(4)))
  {
  }

  /// @brief Construct from a quaternion, normalizing it first and then converting to MRP.
  ///
  /// @param quaternion The source rotation. Singular near a full turn (`w == -1`, where
  /// `vec / (1 + w)` divides by zero); pass the equivalent quaternion with
  /// non-negative `w` to stay well-conditioned. A debug-only assert checks that
  /// the normalized quaternion has unit norm.
  explicit Rotation3(const Quaternion& quaternion):
    Rotation3(
        std::invoke(
            [](const Quaternion& normalizedQuaternion)
            {
              // Eigen's normalized() is accurate to rounding, not exact; compare with its
              // tolerance.
              assert(
                  std::abs(normalizedQuaternion.norm() - Scalar(1)) <=
                  Eigen::NumTraits<Scalar>::dummy_precision());
              return (normalizedQuaternion.vec() / (Scalar(1) + normalizedQuaternion.w())).eval();
            },
            quaternion.normalized()))
  {
  }

  /// @brief Construct from an `Eigen::AngleAxis`.
  explicit Rotation3(const AngleAxis& angleAxis): Rotation3(angleAxis.angle(), angleAxis.axis()) {}

  Rotation3(const Rotation3&) = default;

  Rotation3(Rotation3&&) noexcept = default;

  ~Rotation3() = default;

  Rotation3& operator=(const Rotation3&) = default;

  Rotation3& operator=(Rotation3&&) noexcept = default;

  /// @brief Return a read-only reference to the stored MRP coordinates.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Return a read-only reference to the stored MRP coordinates.
  ///
  /// @see cParameters, parameters (the non-const overload returns a writable reference).
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Return a writable reference to the stored MRP coordinates.
  ///
  /// Writes go directly to storage and are not normalized or validated.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};


} // namespace nioc::geometry

/// @brief A non-owning, mutable view that treats 3 contiguous scalars as a `Rotation3`'s MRP
/// coordinates without copying.
///
/// Example:
///
///     double buffer[3] = {0.0, 0.0, 0.0};
///     Eigen::Map<nioc::geometry::Rotation3<double>> view(buffer);
///     view.parameters() = otherRotation.cParameters();  // writes into buffer.
///
/// The full rotation interface from `Mrp3` operates in place on the pointed-to memory; mutating
/// through this view edits the caller's buffer directly. The caller owns the buffer and must keep
/// it alive and correctly aligned for the lifetime of the map.
///
/// @see Rotation3, Mrp3, Eigen::Map<const Rotation3>
template<typename Scalar_, int MapOptions>
class Eigen::Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>:
  public nioc::geometry::Mrp3<Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>>
{
public:
  using Base = nioc::geometry::Mrp3<Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>>;

  static constexpr auto kDimensions = Base::kDimensions;

  using Scalar = Scalar_;

  using Vector3 [[maybe_unused]] = Matrix<Scalar, kDimensions, 1>;

  using Matrix3 [[maybe_unused]] = Matrix<Scalar, kDimensions, kDimensions>;

  /// The view type over the mapped MRP coordinates: an `Eigen::Map` of a 3-vector.
  using Parameters = Map<Vector3, MapOptions>;

  /// @brief Map the 3 scalars starting at @p ptr.
  ///
  /// @param ptr Points to at least 3 contiguous, correctly aligned scalars that outlive this map.
  explicit Map(Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Return a read-only view of the mapped MRP coordinates.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Return a read-only view of the mapped MRP coordinates.
  ///
  /// @see cParameters, parameters (the non-const overload returns a writable view).
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Return a writable view of the mapped MRP coordinates; writes go to the caller's buffer.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

  /// @brief Convert to an owning `Rotation3` by copying the mapped coordinates.
  ///
  /// NOLINTNEXTLINE(google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};

/// @brief A non-owning, read-only view that treats 3 contiguous const scalars as a `Rotation3`'s
/// MRP coordinates without copying.
///
/// Exposes only the query side of the `Mrp3` interface over the pointed-to memory; there is no
/// mutable parameter access. The caller owns the buffer and must keep it alive and correctly
/// aligned for the lifetime of the map.
///
/// @see Rotation3, Mrp3, Eigen::Map<Rotation3>
template<typename Scalar_, int MapOptions>
class Eigen::Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>:
  public nioc::geometry::Mrp3<Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>>
{
public:
  using Base = nioc::geometry::Mrp3<Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>>;

  static constexpr auto kDimensions = Base::kDimensions;

  using Scalar = Scalar_;

  using Vector3 [[maybe_unused]] = Matrix<Scalar, kDimensions, 1>;

  using Matrix3 [[maybe_unused]] = Matrix<Scalar, kDimensions, kDimensions>;

  /// The view type over the mapped const MRP coordinates: an `Eigen::Map` of a const 3-vector.
  using Parameters = Map<const Vector3, MapOptions>;

  /// @brief Map the 3 const scalars starting at @p ptr.
  ///
  /// @param ptr Points to at least 3 contiguous, correctly aligned scalars that outlive this map.
  explicit Map(const Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Return a read-only view of the mapped MRP coordinates.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Return a read-only view of the mapped MRP coordinates.
  ///
  /// @see cParameters; this view is read-only and offers no mutable overload.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Convert to an owning `Rotation3` by copying the mapped coordinates.
  ///
  /// NOLINTNEXTLINE(google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};
