////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "pose.hpp"
#include "compose.hpp"

namespace naksh::geometry
{

template<typename ParentFrame, typename ChildFrame, typename Scalar_ = double>
class RigidTransform: public FrameReferences<ParentFrame, ChildFrame>
{
public:
    using Scalar = Scalar_;

    using CoordinateFrames = FrameReferences<ParentFrame, ChildFrame>;

    using PoseS = Pose<Scalar>;

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

    const PoseS& pose() const noexcept { return mPose; }

private:
    PoseS mPose;

};


template<typename ParentFrame, typename IntermediateFrame, typename ChildFrame, typename Scalar>
RigidTransform<ParentFrame, ChildFrame, Scalar> operator*(
    const RigidTransform<ParentFrame, IntermediateFrame, Scalar>& lhs,
    const RigidTransform<IntermediateFrame, ChildFrame, Scalar>& rhs)
{
    return RigidTransform<ParentFrame, ChildFrame, Scalar>(
        lhs.pose() * rhs.pose(),
        composeFrameReferences(lhs, rhs));
}

} // End of namespace naksh::geometry.
