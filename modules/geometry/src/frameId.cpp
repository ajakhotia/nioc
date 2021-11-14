////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/geometry/frameId.hpp>

namespace naksh::geometry
{


DynamicFrame::DynamicFrame(std::string frameName) noexcept: mFrameName(std::move(frameName))
{
}


const std::string& DynamicFrame::name() const noexcept
{
    return mFrameName;
}


} // End of namespace naksh::geometry.
