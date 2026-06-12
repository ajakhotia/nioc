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
/// Forward declare Rotation3
template<typename Scalar_ = double>
class Rotation3;

} // namespace nioc::geometry


/// Forward declare Eigen::Map for Rotation3.
template<typename Scalar_, int MapOptions>
class Eigen::Map<nioc::geometry::Rotation3<Scalar_>, MapOptions>;


/// Forward declare Eigen::Map for const Rotation3.
template<typename Scalar_, int MapOptions>
class Eigen::Map<const nioc::geometry::Rotation3<Scalar_>, MapOptions>;

namespace nioc::geometry
{
/// @brief Rotation using modified Rodrigues parameters (MRP).
///
/// Compact 3-parameter representation for rotations. Best for small corrective rotations.
///
/// **Limitations**: Does not handle rotations of magnitude 2(2k+1)π. Use Eigen::Quaternion for
/// general rotations.
///
/// **Reading**: https://link.springer.com/article/10.1007/s10851-017-0765-x#Abs1
///
/// @tparam Derived CRTP derived type.
template<typename Derived>
class Mrp3
{
public:
  static constexpr auto kDimensions = 3U;

  /// @brief Returns a const reference to the derived object.
  /// @return Const reference to the CRTP derived type.
  [[nodiscard]] constexpr const Derived& cDerived() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  /// @brief Returns a const reference to the derived object.
  /// @return Const reference to the CRTP derived type.
  [[nodiscard]] constexpr const Derived& derived() const noexcept
  {
    return cDerived();
  }

  /// @brief Returns a mutable reference to the derived object.
  /// @return Reference to the CRTP derived type.
  constexpr Derived& derived() noexcept
  {
    return static_cast<Derived&>(*this);
  }

  /// @brief Gets pointer to parameter data.
  /// @return Pointer to 3-element parameter array.
  [[nodiscard]] decltype(auto) cData() const noexcept
  {
    return cDerived().cParameters().data();
  }

  /// @brief Gets pointer to parameter data (const).
  /// @return Pointer to parameter array.
  [[nodiscard]] decltype(auto) data() const noexcept
  {
    return cData();
  }

  /// @brief Gets pointer to parameter data (mutable).
  /// @return Pointer to parameter array.
  decltype(auto) data() noexcept
  {
    return derived().parameters().data();
  }

  /// @brief Gets x parameter.
  /// @return X component.
  [[nodiscard]] decltype(auto) x() const noexcept
  {
    return cDerived().cParameters().x();
  }

  /// @brief Gets y parameter.
  /// @return Y component.
  [[nodiscard]] decltype(auto) y() const noexcept
  {
    return cDerived().cParameters().y();
  }

  /// @brief Gets z parameter.
  /// @return Z component.
  [[nodiscard]] decltype(auto) z() const noexcept
  {
    return cDerived().cParameters().z();
  }

  /// @brief Gets x parameter (mutable).
  /// @return X component reference.
  decltype(auto) x() noexcept
  {
    return derived().parameters().x();
  }

  /// @brief Gets y parameter (mutable).
  /// @return Y component reference.
  decltype(auto) y() noexcept
  {
    return derived().parameters().y();
  }

  /// @brief Gets z parameter (mutable).
  /// @return Z component reference.
  decltype(auto) z() noexcept
  {
    return derived().parameters().z();
  }

  /// @brief Gets rotation angle.
  /// @return Angle in radians.
  [[nodiscard]] decltype(auto) angle() const noexcept
  {
    using Scalar = typename Derived::Scalar;
    return Scalar(4) * std::atan(cDerived().cParameters().norm());
  }

  /// @brief Gets rotation axis.
  /// @return Normalized axis vector.
  [[nodiscard]] decltype(auto) axis() const noexcept
  {
    return cDerived().cParameters().normalized().eval();
  }

  /// @brief Computes inverse rotation.
  /// @return Inverted rotation.
  [[nodiscard]] decltype(auto) inverse() const
  {
    return Rotation3<typename Derived::Scalar>(
        typename Derived::Scalar(-1) * cDerived().cParameters());
  }

  /// @brief Composes two rotations.
  /// @param rhsBase Second rotation.
  /// @return Composed rotation.
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

protected:
private:
  Mrp3() = default;

protected:
private:
  Mrp3(const Mrp3&) noexcept = default;

protected:
private:
  Mrp3(Mrp3&&) noexcept = default;

protected:
  ~Mrp3() = default;

  Mrp3& operator=(const Mrp3&) noexcept = default;

  Mrp3& operator=(Mrp3&&) noexcept = default;
  friend Derived;
  friend Derived;
  friend Derived;
};

/// @brief Outputs rotation to stream.
/// @param stream Output stream.
/// @param mrp3 Rotation to output.
/// @return Modified stream.
template<typename Derived>
std::ostream& operator<<(std::ostream& stream, const Mrp3<Derived>& mrp3)
{
  static const auto kIoFormat =
      Eigen::IOFormat(Eigen::FullPrecision, 0, ", ", "\n", "", "", "[", "]");

  stream << mrp3.cDerived().cParameters().transpose().format(kIoFormat);
  return stream;
}

/// @brief 3D rotation with owned storage.
///
/// Stores rotation as 3-element modified Rodrigues parameters.
///
/// @tparam Scalar_ Floating-point type (float, double).
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

  /// @brief Constructs from MRP parameters.
  /// @param parameters Modified Rodrigues parameters (x, y, z).
  explicit Rotation3(Vector3 parameters): mParameters(std::move(parameters)) {}

  /// @brief Constructs from angle-axis representation.
  /// @param angle Rotation angle in radians.
  /// @param axis Rotation axis (normalized automatically).
  Rotation3(const Scalar angle, const Vector3& axis):
    Rotation3(axis.normalized() * std::tan(angle / Scalar(4)))
  {
  }

  /// @brief Constructs from quaternion.
  /// @param quaternion Rotation quaternion (normalized automatically).
  explicit Rotation3(const Quaternion& quaternion):
    Rotation3(std::invoke(
        [](const Quaternion& normalizedQuaternion)
        {
          assert(normalizedQuaternion.norm() == Scalar(1));
          return (normalizedQuaternion.vec() / (Scalar(1) + normalizedQuaternion.w())).eval();
        },
        quaternion.normalized()))
  {
  }

  /// @brief Constructs from Eigen angle-axis.
  /// @param angleAxis Angle-axis rotation.
  explicit Rotation3(const AngleAxis& angleAxis): Rotation3(angleAxis.angle(), angleAxis.axis()) {}

  Rotation3(const Rotation3&) = default;

  Rotation3(Rotation3&&) noexcept = default;

  ~Rotation3() = default;

  Rotation3& operator=(const Rotation3&) = default;

  Rotation3& operator=(Rotation3&&) noexcept = default;

  /// @brief Gets parameters (const).
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Gets parameters (const).
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Gets parameters (mutable).
  /// @return Parameter vector reference.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

private:
  Parameters mParameters;
};


} // namespace nioc::geometry

/// @brief 3D rotation without owned storage.
///
/// Maps existing memory as rotation parameters.
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

  /// @brief Constructs map from pointer.
  /// @param ptr Pointer to 3-element array.
  explicit Map(Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Gets parameters (const).
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Gets parameters (const).
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Gets parameters (mutable).
  /// @return Parameter vector reference.
  Parameters& parameters() noexcept
  {
    return mParameters;
  }

  /// @brief Converts to owned Rotation3.
  /// @return Rotation3 copy.
  /// NOLINTNEXTLINE (google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};

/// @brief Const 3D rotation without owned storage.
///
/// Maps const memory as rotation parameters.
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

  /// @brief Constructs map from const pointer.
  /// @param ptr Pointer to 3-element array.
  explicit Map(const Scalar* ptr): mParameters(ptr) {}

  Map(const Map&) = default;

  Map(Map&&) noexcept = default;

  ~Map() = default;

  Map& operator=(const Map&) = default;

  Map& operator=(Map&&) noexcept = default;

  /// @brief Gets parameters.
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& cParameters() const noexcept
  {
    return mParameters;
  }

  /// @brief Gets parameters.
  /// @return Parameter vector.
  [[nodiscard]] const Parameters& parameters() const noexcept
  {
    return cParameters();
  }

  /// @brief Converts to owned Rotation3.
  /// @return Rotation3 copy.
  /// NOLINTNEXTLINE (google-explicit-constructor)
  operator nioc::geometry::Rotation3<Scalar>() const noexcept
  {
    return nioc::geometry::Rotation3<Scalar>(mParameters);
  }

private:
  Parameters mParameters;
};
