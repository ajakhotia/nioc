////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameConcepts.hpp"

namespace naksh::geometry
{


/// @brief  Base class to provide necessary interface for geometric transformation.
/// @tparam ParentFrameId
/// @tparam ChildFrameId
template<
    typename ParentFrameId,
    typename ChildFrameId,
    typename ParentConcept = ParentConceptTmpl<ParentFrameId>,
    typename ChildConcept = ChildConceptTmpl<ChildFrameId>>
class Transform : public ParentConcept, public ChildConcept
{
public:

    /// @brief  Constructor for when both parent and child frame id's are known statically.
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<
        typename ParentFrame = typename ParentConcept::ParentFrame,
        typename ChildFrame = typename ChildConcept::ChildFrame,
        typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
        typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
    Transform() noexcept: ParentConcept(), ChildConcept()
    {
    }


    /// @brief  Constructor for when the parent frame is known statically but the child frame is dynamic.
    /// @tparam ChildConceptArgs    Argument type to initialize the child frame. Typically this is either a
    ///                             std::string("...") or DynamicFrame("...")
    /// @tparam ParentFrame
    /// @tparam ChildFrame
    template<
        typename ChildConceptArgs,
        typename ParentFrame = typename ParentConcept::ParentFrame,
        typename ChildFrame = typename ChildConcept::ChildFrame,
        typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
        typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
    [[maybe_unused]] explicit Transform(ChildConceptArgs&& childId) noexcept:
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
        typename ParentFrame = typename ParentConcept::ParentFrame,
        typename ChildFrame = typename ChildConcept::ChildFrame,
        typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
        typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
    [[maybe_unused]] explicit Transform(ParentConceptArgs&& parentId, int = 0) noexcept:
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
    template<
        typename ParentConceptArgs,
        typename ChildConceptArgs,
        typename ParentFrame = typename ParentConcept::ParentFrame,
        typename ChildFrame = typename ChildConcept::ChildFrame,
        typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
        typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
    [[maybe_unused]] Transform(ParentConceptArgs&& parentId, ChildConceptArgs&& childId) noexcept:
        ParentConcept(std::forward<ParentConceptArgs>(parentId)),
        ChildConcept(std::forward<ChildConceptArgs>(childId))
    {
    }


    /// @brief Default virtual destructor.
    virtual ~Transform() = default;
};


} // End of namespace naksh::geometry.
