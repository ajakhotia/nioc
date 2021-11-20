////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "transform.hpp"

namespace naksh::geometry
{


template<typename LhsTransform, typename RhsTransform>
class ComposeTransform
{
    static_assert(
        FramesEqual<typename LhsTransform::ChildFrame, typename RhsTransform::ParentFrame>::value(),
        "ChildFrame of Lhs does not match the ParentFrame for Rhs. The transforms "
        "cannot be composed together.");

    using ResultTransform = Transform<typename LhsTransform::ParentFrame, typename RhsTransform::ChildFrame>;
};

} // End of namespace naksh::geometry.
