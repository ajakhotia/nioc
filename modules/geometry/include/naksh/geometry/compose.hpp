////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "transform.hpp"
#include <tuple>
#include <cassert>

namespace naksh::geometry
{


/// @brief  Error thrown when a mis-match of frame ids is detected during transform
///         composition at run-time.
class TransformCompositionException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;

    /// @brief  Convenience constructor to appropriately build error message from frame names.
    /// @param lhsFrameName
    /// @param rhsFrameName
    TransformCompositionException(const std::string & lhsFrameName, const std::string& rhsFrameName) noexcept:
        TransformCompositionException(
            "Composed transforms with mismatched inner frames. Lhs child frame[" +
            lhsFrameName + "] does not match the rhs parent frame[" + rhsFrameName + "].")
    {
        assert(lhsFrameName != rhsFrameName &&
            "Detected creation of TransformCompositionException but with compatible frame. "
            "This likely a programming error.");
    }
};


namespace helpers
{


template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and
        common::isSpecialization<RhsFrame, StaticFrame> and
        LhsFrame::name() == RhsFrame::name()>>
inline constexpr void assertFrameEqual() noexcept
{}


template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and
        std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const RhsFrame& rhsFrame)
{
    if(LhsFrame::name() != rhsFrame.name())
    {
        throw TransformCompositionException(LhsFrame::name(), rhsFrame.name());
    }
}


template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and
        common::isSpecialization<RhsFrame, StaticFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame)
{
    if(lhsFrame.name() != RhsFrame::name())
    {
        throw TransformCompositionException(lhsFrame.name(), RhsFrame::name());
    }
}


template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and
        std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame, const RhsFrame& rhsFrame)
{
    if(lhsFrame.name() != rhsFrame.name())
    {
        throw TransformCompositionException(lhsFrame.name(), rhsFrame.name());
    }
}


} // End of namespace helpers.


/// @brief
/// @tparam LhsTransform
/// @tparam RhsTransform
/// @param lhsTransform
/// @param rhsTransform
/// @return
template<typename LhsTransform, typename RhsTransform>
decltype(auto) composeTransform(const LhsTransform& lhsTransform, const RhsTransform& rhsTransform)
{
    std::apply(
        helpers::assertFrameEqual<typename LhsTransform::ChildFrame, typename RhsTransform::ParentFrame>,
        std::tuple_cat(lhsTransform.childFrameAsTuple(), rhsTransform.parentFrameAsTuple()));

    return std::make_from_tuple<
        Transform<typename LhsTransform::ParentFrame, typename RhsTransform::ChildFrame>>(
            std::tuple_cat(lhsTransform.parentFrameAsTuple(), rhsTransform.childFrameAsTuple()));
}


} // End of namespace naksh::geometry.
