////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameReferences.hpp"
#include <naksh/common/typeTraits.hpp>
#include <string>
#include <cassert>
#include <type_traits>
#include <tuple>

namespace naksh::geometry
{


/// @brief  Error thrown when a mis-match of frame ids is detected during transform
///         composition at run-time.
class FrameCompositionException : public std::runtime_error
{
public:
    /// Inherit all constructors from std::runtime_error.
    using std::runtime_error::runtime_error;

    /// @brief  Convenience constructor to appropriately build error message from frame names.
    /// @param lhsFrameName
    /// @param rhsFrameName
    FrameCompositionException(const std::string & lhsFrameName, const std::string& rhsFrameName) noexcept:
        FrameCompositionException(
            "Composed transforms with mismatched inner frames. Lhs child frame[" +
            lhsFrameName + "] does not match the rhs parent frame[" + rhsFrameName + "].")
    {
        assert(lhsFrameName != rhsFrameName &&
            "Detected creation of FrameCompositionException but with compatible frame ids. "
            "This likely a programming error.");
    }
};


namespace helpers
{

/// @brief  Asserts equality of lhs and rhs frame for the case when both are
///         instances of StaticFrame<>. Because the equality is evaluable at
///         compile-time, no run-time work in needed for this assertion.
/// @tparam LhsFrame
/// @tparam RhsFrame
template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and
        common::isSpecialization<RhsFrame, StaticFrame> and
        LhsFrame::name() == RhsFrame::name()>>
inline constexpr void assertFrameEqual() noexcept
{}


/// @brief  Asserts equality of lhs and rhs frame for the case when Lhs is a StaticFrame<...>
///         and Rhs is a DynamicFrame.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param rhsFrame
template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and
        std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const RhsFrame& rhsFrame)
{
    if(LhsFrame::name() != rhsFrame.name())
    {
        throw FrameCompositionException(std::string(LhsFrame::name()), rhsFrame.name());
    }
}


/// @brief  Asserts equality of lhs and rhs frame for the case when Lhs is a DynamicFrame
///         and Rhs is a StaticFrame<...>.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param lhsFrame
template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and
        common::isSpecialization<RhsFrame, StaticFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame)
{
    if(lhsFrame.name() != RhsFrame::name())
    {
        throw FrameCompositionException(lhsFrame.name(), std::string(RhsFrame::name()));
    }
}


/// @brief  Asserts equality of lhs and rhs frame for the case when they both
///         DynamicFrame.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param lhsFrame
/// @param rhsFrame
template<typename LhsFrame, typename RhsFrame, typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and
        std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame, const RhsFrame& rhsFrame)
{
    if(lhsFrame.name() != rhsFrame.name())
    {
        throw FrameCompositionException(lhsFrame.name(), rhsFrame.name());
    }
}


} // End of namespace helpers.


/// @brief  Composes transforms together and returns appropriate result. Throws
///         FrameCompositionException if the transform are not composable,
///         i.e the ChildFrame of Lhs does not match the ParentFrame of Rhs.
/// @tparam LhsFrameReferences    Type of the Lhs.
/// @tparam RhsFrameReferences    Type of the Rhs.
/// @param lhsFrameReferences     Instance of the Lhs.
/// @param rhsFrameReferences     Instance of the Rhs.
/// @return FrameReferences that are a result of the composition.
template<typename LhsFrameReferences, typename RhsFrameReferences>
decltype(auto) composeFrameReferences(
    const LhsFrameReferences& lhsFrameReferences, const RhsFrameReferences& rhsFrameReferences)
{
    std::apply(
        helpers::assertFrameEqual<typename LhsFrameReferences::ChildFrame, typename RhsFrameReferences::ParentFrame>,
        std::tuple_cat(lhsFrameReferences.childFrameAsTuple(), rhsFrameReferences.parentFrameAsTuple()));

    return std::make_from_tuple<
        FrameReferences<typename LhsFrameReferences::ParentFrame, typename RhsFrameReferences::ChildFrame>>(
            std::tuple_cat(lhsFrameReferences.parentFrameAsTuple(), rhsFrameReferences.childFrameAsTuple()));
}


} // End of namespace naksh::geometry.
