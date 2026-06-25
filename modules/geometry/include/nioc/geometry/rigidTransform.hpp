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
/// @brief An SE(3) rigid-body transform tagged with the parent and child frames it relates.
///
/// Pairs a Pose (orientation plus position) with a typed parent->child frame relationship, so the
/// frames a transform connects are part of its type. Read an instance as "the pose of @p ChildFrame
/// expressed in @p ParentFrame". Multiplying two transforms chains their frames like a matrix
/// product; the frame tags reject a mismatched chain at compile time for static frames or at
/// runtime for dynamic frames.
///
/// Example:
///
///     // Static parent and child frames: no runtime ids needed.
///     RigidTransform<World, Robot> worldFromRobot{pose};
///
///     // A dynamic child frame: pass its runtime id.
///     RigidTransform<World, DynamicFrame> worldFromSensor{pose, "sensor"};
///
/// @tparam ParentFrame The source frame. A StaticFrame<> specialization or DynamicFrame.
///
/// @tparam ChildFrame The target frame. A StaticFrame<> specialization or DynamicFrame.
///
/// @tparam Scalar_ The pose's number type. Defaults to double.
///
/// @see operator*, FrameReferences
template<typename ParentFrame, typename ChildFrame, typename Scalar_ = double>
class RigidTransform: public FrameReferences<ParentFrame, ChildFrame>
{
public:
  using Scalar = Scalar_;

  /// The frame-tag base type carrying the parent->child frame relationship.
  using CoordinateFrames = FrameReferences<ParentFrame, ChildFrame>;

  /// The untagged pose type this transform holds.
  using PoseS = Pose<Scalar>;

  /// @brief Construct from a pose plus one runtime id per dynamic frame end.
  ///
  /// @tparam FrameRefParams The runtime id types forwarded to the frame-tag base, one per dynamic
  /// frame end.
  ///
  /// @param pose The orientation and position, untagged.
  ///
  /// @param frameRefParams The frame ids identifying dynamic ends. Pass none when both frames are
  /// static, otherwise one id per DynamicFrame end, parent before child.
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

  /// @brief The underlying untagged pose.
  ///
  /// @return A reference valid for the transform's lifetime.
  [[nodiscard]] const PoseS& pose() const noexcept
  {
    return mPose;
  }

  /// @brief A fresh transform in the opposite direction, mapping the child frame back to the
  /// parent; this object is unchanged.
  [[nodiscard]] RigidTransform<ChildFrame, ParentFrame, Scalar> inverse() const
  {
    return RigidTransform<ChildFrame, ParentFrame, Scalar>(
        pose().inverse(),
        invertFrameReferences(*this));
  }

private:
  /// The untagged orientation and position this transform wraps, returned by pose().
  PoseS mPose;
};

/// @brief Chain two transforms that meet at a shared intermediate frame into one parent->child
/// transform.
///
/// Models composing @p lhs (parent -> intermediate) with @p rhs (intermediate -> child); the result
/// relates @p lhs's parent to @p rhs's child. The shared @p IntermediateFrame must be @p lhs's
/// child and @p rhs's parent: for static frames a type mismatch is a compile error, for dynamic
/// frames the names are compared at runtime. Both operands must share @p Scalar.
///
/// Example:
///
///     RigidTransform<World, Child> worldFromChild = worldFromRobot * robotFromChild;
///
/// @throws FrameCompositionException When a dynamic intermediate frame's parent and child names
/// differ.
///
/// @see RigidTransform
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
