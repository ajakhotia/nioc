////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameReferences.hpp"
#include "pose.hpp"

namespace nioc::geometry
{
/// @brief Transformation between two coordinate frames.
///
/// Combines pose with frame relationship. Represents how to transform from child to parent frame.
///
/// @tparam ParentFrame Parent coordinate frame type.
/// @tparam ChildFrame Child coordinate frame type.
/// @tparam Scalar_ Floating-point type (default: double).
template<typename ParentFrame, typename ChildFrame, typename Scalar_ = double>
class RigidTransform: public FrameReferences<ParentFrame, ChildFrame>
{
public:
  using Scalar = Scalar_;

  using CoordinateFrames = FrameReferences<ParentFrame, ChildFrame>;

  using PoseS = Pose<Scalar>;

  /// @brief Constructs a transform with pose and frame identities.
  /// @param pose Transformation pose.
  /// @param frameRefParams Frame identity parameters.
  template<typename... FrameRefParams>
  explicit RigidTransform(const PoseS& pose, FrameRefParams&&... frameRefParams):
      CoordinateFrames(std::forward<FrameRefParams>(frameRefParams)...),
      mPose(pose)
  {
  }

  RigidTransform(const RigidTransform&) = default;

  RigidTransform(RigidTransform&&) noexcept = default;

  ~RigidTransform() = default;

  RigidTransform& operator=(const RigidTransform&) = default;

  RigidTransform& operator=(RigidTransform&&) noexcept = default;

  /// @brief Gets the transformation pose.
  /// @return Reference to the pose.
  const PoseS& pose() const noexcept
  {
    return mPose;
  }

  /// @brief Computes the inverse transformation.
  /// @return Transform from parent to child frame.
  RigidTransform<ChildFrame, ParentFrame, Scalar> inverse() const
  {
    return RigidTransform<ChildFrame, ParentFrame, Scalar>(
        pose().inverse(),
        invertFrameReferences(*this));
  }

private:
  PoseS mPose;
};

/// @brief Chains two transformations together.
/// @return Composed transformation from child to parent frame.
template<typename ParentFrame, typename IntermediateFrame, typename ChildFrame, typename Scalar>
RigidTransform<ParentFrame, ChildFrame, Scalar> operator*(
    const RigidTransform<ParentFrame, IntermediateFrame, Scalar>& lhs,
    const RigidTransform<IntermediateFrame, ChildFrame, Scalar>& rhs)
{
  return RigidTransform<ParentFrame, ChildFrame, Scalar>(
      lhs.pose() * rhs.pose(),
      composeFrameReferences(lhs, rhs));
}

} // namespace nioc::geometry
