////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameConcepts.hpp"

#include <naksh/common/typeTraits.hpp>
#include <type_traits>

namespace naksh::geometry
{

/// @brief  Base class to provide necessary interface for geometric transformation.
/// @tparam ParentFrame
/// @tparam ChildFrame
template<typename ParentFrame_,
         typename ChildFrame_,
         typename ParentConcept = ParentConceptTmpl<ParentFrame_>,
         typename ChildConcept = ChildConceptTmpl<ChildFrame_>>
class FrameReferences: public ParentConcept, public ChildConcept
{
public:
    using SelfType = FrameReferences<ParentFrame_, ChildFrame_>;

    /// @brief  Constructor for when both parent and child frame id's are known statically.
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<
        typename ParentFrame = typename SelfType::ParentFrame,
        typename ChildFrame = typename SelfType::ChildFrame,
        typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
        typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
    FrameReferences() noexcept: ParentConcept(), ChildConcept()
    {
    }


    /// @brief  Constructor for when the parent frame is known statically but the child frame is dynamic.
    /// @tparam ChildConceptArgs    Argument type to initialize the child frame. Typically this is either a
    ///                             std::string("...") or DynamicFrame("...")
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<
        typename ChildConceptArgs,
        typename ParentFrame = typename SelfType::ParentFrame,
        typename ChildFrame = typename SelfType::ChildFrame,
        typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
        typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
    [[maybe_unused]] explicit FrameReferences(ChildConceptArgs&& childId) noexcept:
        ParentConcept(), ChildConcept(std::forward<ChildConceptArgs>(childId))
    {
    }


    /// @brief  Constructor for when the parent frame is dynamic but the child frame is known statically.
    /// @tparam ParentConceptArgs   Argument type to initialize the parent frame. Typically this is either a
    ///                             std::string("...") or DynamicFrame("...")
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<
        typename ParentConceptArgs,
        typename ParentFrame = typename SelfType::ParentFrame,
        typename ChildFrame = typename SelfType::ChildFrame,
        typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
        typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
    [[maybe_unused]] explicit FrameReferences(ParentConceptArgs&& parentId, int = 0) noexcept:
        ParentConcept(std::forward<ParentConceptArgs>(parentId)), ChildConcept()
    {
    }


    /// @brief  Constructor for when both the parent and the child frame are known dynamically.
    /// @tparam ParentConceptArgs   Argument type to initialize the parent frame. Typically this is either a
    ///                             std::string("...") or DynamicFrame("...")
    /// @tparam ChildConceptArgs    Argument type to initialize the child frame. Typically this is either a
    ///                             std::string("...") or DynamicFrame("...")
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<typename ParentConceptArgs,
             typename ChildConceptArgs,
             typename ParentFrame = typename SelfType::ParentFrame,
             typename ChildFrame = typename SelfType::ChildFrame,
             typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
             typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
    [[maybe_unused]] FrameReferences(ParentConceptArgs&& parentId,
                                     ChildConceptArgs&& childId) noexcept:
        ParentConcept(std::forward<ParentConceptArgs>(parentId)),
        ChildConcept(std::forward<ChildConceptArgs>(childId))
    {
    }


    /// @brief Default virtual destructor.
    virtual ~FrameReferences() = default;
};


/// @brief  Error thrown when a mis-match of frame ids is detected during transform
///         composition at run-time.
class FrameCompositionException: public std::runtime_error
{
public:
    /// Inherit all constructors from std::runtime_error.
    using std::runtime_error::runtime_error;
};


namespace helpers
{

std::string frameCompositionErrorMessage(const std::string& lhsFrameName,
                                         const std::string& rhsFrameName);


/// @brief  Asserts equality of lhs and rhs frame for the case when both are
///         instances of StaticFrame<>. Because the equality is evaluable at
///         compile-time, no run-time work in needed for this assertion.
/// @tparam LhsFrame
/// @tparam RhsFrame
template<typename LhsFrame,
         typename RhsFrame,
         typename = typename std::enable_if_t<common::isSpecialization<LhsFrame, StaticFrame> and
                                              common::isSpecialization<RhsFrame, StaticFrame> and
                                              LhsFrame::name() == RhsFrame::name()>>
inline constexpr void assertFrameEqual() noexcept
{
}


/// @brief  Asserts equality of lhs and rhs frame for the case when Lhs is a StaticFrame<...>
///         and Rhs is a DynamicFrame.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param rhsFrame
template<typename LhsFrame,
         typename RhsFrame,
         typename = typename std::enable_if_t<common::isSpecialization<LhsFrame, StaticFrame> and
                                              std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const RhsFrame& rhsFrame)
{
    if(LhsFrame::name() != rhsFrame.name())
    {
        auto msg = frameCompositionErrorMessage(std::string(LhsFrame::name()), rhsFrame.name());
        throw FrameCompositionException(std::move(msg));
    }
}


/// @brief  Asserts equality of lhs and rhs frame for the case when Lhs is a DynamicFrame
///         and Rhs is a StaticFrame<...>.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param lhsFrame
template<typename LhsFrame,
         typename RhsFrame,
         typename = typename std::enable_if_t<std::is_same_v<LhsFrame, DynamicFrame> and
                                              common::isSpecialization<RhsFrame, StaticFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame)
{
    if(lhsFrame.name() != RhsFrame::name())
    {
        auto msg = frameCompositionErrorMessage(lhsFrame.name(), std::string(RhsFrame::name()));
        throw FrameCompositionException(std::move(msg));
    }
}


/// @brief  Asserts equality of lhs and rhs frame for the case when they both
///         DynamicFrame.
/// @tparam LhsFrame
/// @tparam RhsFrame
/// @param lhsFrame
/// @param rhsFrame
template<typename LhsFrame,
         typename RhsFrame,
         typename = typename std::enable_if_t<std::is_same_v<LhsFrame, DynamicFrame> and
                                              std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame, const RhsFrame& rhsFrame)
{
    if(lhsFrame.name() != rhsFrame.name())
    {
        auto msg = frameCompositionErrorMessage(lhsFrame.name(), rhsFrame.name());
        throw FrameCompositionException(std::move(msg));
    }
}


} // namespace helpers


/// @brief  Composes transforms together and returns appropriate result. Throws
///         FrameCompositionException if the transform are not composable,
///         i.e the ChildFrame of Lhs does not match the ParentFrame of Rhs.
/// @tparam LhsFrameReferences    Type of the Lhs.
/// @tparam RhsFrameReferences    Type of the Rhs.
/// @param lhsFrameReferences     Instance of the Lhs.
/// @param rhsFrameReferences     Instance of the Rhs.
/// @return FrameReferences that are a result of the composition.
template<typename LhsFrameReferences, typename RhsFrameReferences>
constexpr FrameReferences<typename LhsFrameReferences::ParentFrame,
                          typename RhsFrameReferences::ChildFrame>
composeFrameReferences(const LhsFrameReferences& lhsFrameReferences,
                       const RhsFrameReferences& rhsFrameReferences)
{
    using ResultFrameReference = FrameReferences<typename LhsFrameReferences::ParentFrame,
                                                 typename RhsFrameReferences::ChildFrame>;

    std::apply(helpers::assertFrameEqual<typename LhsFrameReferences::ChildFrame,
                                         typename RhsFrameReferences::ParentFrame>,
               std::tuple_cat(lhsFrameReferences.childFrameAsTuple(),
                              rhsFrameReferences.parentFrameAsTuple()));

    return std::make_from_tuple<ResultFrameReference>(std::tuple_cat(
        lhsFrameReferences.parentFrameAsTuple(), rhsFrameReferences.childFrameAsTuple()));
}


/// @brief
/// @tparam InputFrameReferences
/// @param input
/// @return
template<typename InputFrameReferences>
FrameReferences<typename InputFrameReferences::ChildFrame,
                typename InputFrameReferences::ParentFrame>
invertFrameReferences(const InputFrameReferences& input)
{
    using Result = FrameReferences<typename InputFrameReferences::ChildFrame,
                                   typename InputFrameReferences::ParentFrame>;

    return std::make_from_tuple<Result>(
        std::tuple_cat(input.childFrameAsTuple(), input.parentFrameAsTuple()));
}


} // namespace naksh::geometry
