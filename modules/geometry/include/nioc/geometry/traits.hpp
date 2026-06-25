////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace nioc::geometry
{
/// @brief Trait map from a geometry type to its associated types, such as its `Scalar`, for use by
/// generic code.
///
/// CRTP bases like `Se3` read a derived type's dependent types through `Traits<Derived>` instead of
/// from the derived class directly, which avoids needing those types to be defined before the base
/// is instantiated. To make a new type work with such a base, specialize this template for that
/// type and add the member type aliases the base needs.
///
/// Example:
///
///     // For a type MyPose, expose its scalar so Se3<MyPose> can read it.
///     template<typename Scalar_>
///     struct Traits<MyPose<Scalar_>>
///     {
///       using Scalar = Scalar_;
///     };
///
/// The primary template is declared but never defined, so it is incomplete: naming any member of an
/// unspecialized `Traits` is a compile error. That error is the intended signal that a type is
/// missing its specialization. Each concrete type specializes `Traits` for itself and for its
/// `Eigen::Map` wrappers.
///
/// @see Pose, Se3
template<typename>
struct Traits;

} // namespace nioc::geometry
