////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/geometry/frame.hpp>

namespace naksh::geometry
{

DynamicFrame::DynamicFrame(std::string frameId) noexcept : mFrameId(std::move(frameId))
{
}


[[nodiscard]] const std::string& DynamicFrame::name() const noexcept
{
    return mFrameId;
}


bool operator==(const DynamicFrame& lhs, const DynamicFrame& rhs)
{
    return lhs.name() == rhs.name();
}


bool operator!=(const DynamicFrame& lhs, const DynamicFrame& rhs)
{
    return lhs.name() != rhs.name();
}

} // End of namespace naksh::geometry.
