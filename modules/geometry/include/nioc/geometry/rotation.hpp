////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <ostream>
#include <utility>

namespace nioc::geometry
{
/// Forward declaration.
template<typename Scalar_ = double>
class Rotation3;

} // namespace nioc::geometry


/// Forward declaration.
template<typename Scalar_, int MapOptions>
class Eigen::Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>;


/// Forward declaration.
template<typename Scalar_, int MapOptions>
class Eigen::Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>;

namespace nioc::geometry
{
/// @brief A rotation stored as 3 modified Rodrigues parameters (MRP).
///
/// Compact and best for small rotations.
///
/// **Limitations**: cannot represent rotations of magnitude 2(2k+1)π. Use Eigen::Quaternion for
/// those.
///
/// **Reading**: https://link.springer.com/article/10.1007/s10851-017-0765-x#Abs1
///
/// @tparam Derived The CRTP subclass that holds the parameters.
template<typename Derived>
class Mrp3
{
public:
  static constexpr auto kDimensions = 3U;

  /// @brief Returns a const reference to the derived object.
  [[nodiscard]] constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  /// @brief Returns a const reference to the derived object.
  [[nodiscard]] constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  /// @brief Returns a mutable reference to the derived object.
  constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }

  /// @brief Returns a const pointer to the 3-element parameter array.
  [[nodiscard]] decltype(auto) cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  /// @brief Returns a const pointer to the 3-element parameter array.
  [[nodiscard]] decltype(auto) data() const noexcept
  {
    return cData();
  }

  /// @brief Returns a mutable pointer to the 3-element parameter array.
  decltype(auto) data() noexcept
  {
    return derived().parameters().data();
  }

  /// @brief Returns the x parameter.
  [[nodiscard]] decltype(auto) x() const noexcept
  {
    return cDerived().cParameters().x();
  }

  /// @brief Returns the y parameter.
  [[nodiscard]] decltype(auto) y() const noexcept
  {
    return cDerived().cParameters().y();
  }

  /// @brief Returns the z parameter.
  [[nodiscard]] decltype(auto) z() const noexcept
  {
    return cDerived().cParameters().z();
  }

  /// @brief Returns a mutable reference to the x parameter.
  decltype(auto) x() noexcept
  {
    return derived().parameters().x();
  }

  /// @brief Returns a mutable reference to the y parameter.
  decltype(auto) y() noexcept
  {
    return derived().parameters().y();
  }

  /// @brief Returns a mutable reference to the z parameter.
  decltype(auto) z() noexcept
  {
    return derived().parameters().z();
  }

  /// @brief Returns the rotation angle, in radians.
  [[nodiscard]] decltype(auto) angle() const noexcept
  {
    using Scalar = typename Derived::Scalar;
    return Scalar(4) * std::atan(cDerived().cParameters().norm());
  }

  /// @brief Returns the rotation axis, normalized to unit length.
  ///
  /// Returns `auto`, not `decltype(auto)`: eval() yields a reference into a temporary, so deducing
  /// a reference would dangle. A value copies before the temporary dies.
  [[nodiscard]] auto axis() const noexcept
  {
    return cDerived().cParameters().normalized().eval();
  }

  /// @brief Returns the inverse rotation.
  [[nodiscard]] decltype(auto) inverse() const
  {
    return Rotation3<typename Derived::Scalar>(
        typename Derived::Scalar(-1) * cDerived().cParameters());
  }

  /// @brief Composes two rotations. The result applies @p rhsBase first, then this rotation.
  /// @param rhsBase The rotation applied first.
  /// @return The combined rotation.
  decltype(auto) operator*(const Mrp3& rhsBase) const
  {
    using Scalar = typename Derived::Scalar;

    const auto& lhsMrp = this->cDerived().cParameters();
    const auto& rhsMrp = rhsBase.cDerived().cParameters();

    const auto lhsNorm2 = lhsMrp.squaredNorm();
    const auto rhsNorm2 = rhsMrp.squaredNorm();

    const auto vec = ((Scalar(1) - rhsNorm2) * lhsMrp) + ((Scalar(1) - lhsNorm2) * rhsMrp) +
                     (Scalar(2) * lhsMrp.cross(rhsMrp));

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

/// @brief Writes the rotation's parameters to a stream.
/// @param stream The stream to write to.
/// @param mrp3 The rotation to write.
/// @return The same stream.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Mrp3<Derived>& mrp3)
{
  static const auto kIoFormat =
      Eigen::IOFormat(Eigen::FullPrecision, 0, ", ", "\n", "", "", "[", "]");

  stream << mrp3.cDerived().cParameters().transpose().format(kIoFormat);
  return stream;
}

/// @brief A 3D rotation that owns its 3 MRP storage values.
///
/// @tparam Scalar_ Floating-point type (float or double).
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

  using Parameters = Vector3;

  /// @brief Constructs from MRP values.
  /// @param parameters The (x, y, z) modified Rodrigues parameters.
  explicit Rotation3(Vector3 parameters): mParameters(std::move(parameters)) {}

  /// @brief Constructs from an angle and axis.
  /// @param angle Rotation angle, in radians.
  /// @param axis Rotation axis. Normalized for you.
  Rotation3(const Scalar angle, const Vector3& axis):
    Rotation3(axis.normalized() * std::tan(angle / Scalar(4)))
  {
  }

  /// @brief Constructs from a quaternion.
  /// @param quaternion Rotation quaternion. Normalized for you.
  explicit Rotation3(const Quaternion& quaternion):
    Rotation3(std::invoke(
        [](const Quaternion& normalizedQuaternion)
        {
          // Eigen's normalized() is accurate to rounding, not exact; compare with its tolerance.
          assert(
              std::abs(normalizedQuaternion.norm() - Scalar(1)) <=
              Eigen::NumTraits<Scalar>::dummy_precision());
          return (normalizedQuaternion.vec() / (Scalar(1) + normalizedQuaternion.w())).eval();
        },
        quaternion.normalized()))
  {
  }

  /// @brief Constructs from an Eigen angle-axis.
  /// @param angleAxis The angle-axis rotation.
  explicit Rotation3(const AngleAxis& angleAxis): Rotation3(angleAxis.angle(), angleAxis.axis()) {}

  Rotation3(const Rotation3&) = default;

  Rotation3(Rotation3&&) noexcept = default;

  ~Rotation3() = default;

  Rotation3& operator=(const Rotation3&) = default;

  Rotation3& operator=(Rotation3&&) noexcept = default;

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Returns a mutable reference to the parameter vector.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};


} // namespace nioc::geometry

/// @brief A 3D rotation that views existing memory instead of owning it.
///
/// Reads and writes the rotation parameters in place.
///
/// @tparam Scalar_ Floating-point type.
/// @tparam MapOptions Eigen map options.
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

  using Parameters = Map<Vector3, MapOptions>;

  /// @brief Constructs a view over an existing 3-element array.
  /// @param ptr Pointer to the 3-element array. Must stay alive while this map is used.
  explicit Map(Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Returns a mutable reference to the parameter vector.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

  /// @brief Copies the viewed values into an owned Rotation3.
  /// NOLINTNEXTLINE (google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};

/// @brief A read-only 3D rotation that views existing memory instead of owning it.
///
/// Reads the rotation parameters in place. Cannot modify them.
///
/// @tparam Scalar_ Floating-point type.
/// @tparam MapOptions Eigen map options.
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

  using Parameters = Map<const Vector3, MapOptions>;

  /// @brief Constructs a read-only view over an existing 3-element array.
  /// @param ptr Pointer to the 3-element array. Must stay alive while this map is used.
  explicit Map(const Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Returns a const reference to the parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Copies the viewed values into an owned Rotation3.
  /// NOLINTNEXTLINE (google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};
