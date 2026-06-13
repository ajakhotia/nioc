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
/// @brief A pose that transforms points from the child frame to the parent frame.
///
/// @tparam ParentFrame Parent frame type.
/// @tparam ChildFrame Child frame type.
/// @tparam Scalar_ Floating-point type (default: double).
template<typename ParentFrame, typename ChildFrame, typename Scalar_ = double>
class RigidTransform: public FrameReferences<ParentFrame, ChildFrame>
{
public:
  using Scalar = Scalar_;

  using CoordinateFrames = FrameReferences<ParentFrame, ChildFrame>;

  using PoseS = Pose<Scalar>;

  /// @brief Builds the transform from a pose and the frame identities.
  /// @param pose The child-to-parent pose.
  /// @param frameRefParams Arguments that name the parent and child frames.
  template<typename... FrameRefParams>
  explicit RigidTransform(const PoseS& pose, FrameRefParams&&... frameRefParams):
    CoordinateFrames(std::forward<FrameRefParams>(frameRefParams)...),
    mPose(pose)
  {
  }

  RigidTransform(const RigidTransform&) = default;

  RigidTransform(RigidTransform&&) noexcept = default;

  ~RigidTransform() override = default;

  RigidTransform& operator=(const RigidTransform&) = default;

  RigidTransform& operator=(RigidTransform&&) noexcept = default;

  /// @brief Returns the child-to-parent pose.
  [[nodiscard]] const PoseS& pose() const noexcept
  {
    return mPose;
  }

  /// @brief Returns the inverse transform.
  /// @return The transform from parent to child frame.
  [[nodiscard]] RigidTransform<ChildFrame, ParentFrame, Scalar> inverse() const
  {
    return RigidTransform<ChildFrame, ParentFrame, Scalar>(
        pose().inverse(),
        invertFrameReferences(*this));
  }

private:
  PoseS mPose;
};

/// @brief Chains two transforms.
/// @return The combined transform from child to parent frame.
/// @throws FrameCompositionException if the intermediate frames have mismatched runtime names.
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
