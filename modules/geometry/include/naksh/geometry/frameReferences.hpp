////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frame.hpp"

namespace naksh::geometry
{
namespace helpers
{


/// @brief  A class representing the concept of a parent frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename FrameId>
class ParentConcept
{
public:
    /// Avoid nesting StaticFrame type
    static_assert(not common::IsSpecialization<FrameId, StaticFrame>::value);

    using ParentFrame = StaticFrame<FrameId>;

    virtual ~ParentConcept() = default;
};


/// @brief  A class representing the concept of a child frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename FrameId>
class ChildConcept
{
public:
    /// Avoid nesting StaticFrame type
    static_assert(not common::IsSpecialization<FrameId, StaticFrame>::value);

    using ChildFrame = StaticFrame<FrameId>;

    virtual ~ChildConcept() = default;
};


/// @brief  ParentConcept specialized for the dynamic case.
template<>
class ParentConcept<DynamicFrame>
{
public:
    using ParentFrame = DynamicFrame;

    explicit ParentConcept(ParentFrame parent) noexcept: mParentFrame(std::move(parent))
    {
    }

    explicit ParentConcept(std::string frameId) noexcept:
            ParentConcept(ParentFrame(std::move(frameId)))
    {
    }

    virtual ~ParentConcept() = default;

    [[nodiscard]] const ParentFrame& parentFrame() const noexcept
    { return mParentFrame; }

private:
    ParentFrame mParentFrame;
};


/// @brief  ChildConcept specialized for the dynamic case.
template<>
class ChildConcept<DynamicFrame>
{
public:
    using ChildFrame = DynamicFrame;

    explicit ChildConcept(ChildFrame child) noexcept: mChildFrame(std::move(child))
    {
    }

    explicit ChildConcept(std::string frameId) noexcept:
            ChildConcept(ChildFrame(std::move(frameId)))
    {
    }

    virtual ~ChildConcept() = default;

    [[nodiscard]] const ChildFrame& childFrame() const noexcept
    { return mChildFrame; }

private:
    ChildFrame mChildFrame;
};


} // End of namespace helpers.


/// @brief
/// @tparam ParentFrameId
/// @tparam ChildFrameId
template<typename ParentFrameId, typename ChildFrameId>
class FrameReferences : public helpers::ParentConcept<ParentFrameId>, public helpers::ChildConcept<ChildFrameId>
{
public:

    template<
            typename PConcept = helpers::ParentConcept<ParentFrameId>,
            typename CConcept = helpers::ChildConcept<ChildFrameId>,
            typename = typename std::enable_if<
                    common::IsSpecialization<typename PConcept::ParentFrame, StaticFrame>::value>::type,
            typename = typename std::enable_if<
                    common::IsSpecialization<typename CConcept::ChildFrame, StaticFrame>::value>::type
                    >
    FrameReferences():
            helpers::ParentConcept<ParentFrameId>(),
            helpers::ChildConcept<ChildFrameId>()
    {
    }


    template<
            typename ChildArg,
            typename PConcept = helpers::ParentConcept<ParentFrameId>,
            typename CConcept = helpers::ChildConcept<DynamicFrame>,
            typename = typename std::enable_if<
                    common::IsSpecialization<typename PConcept::ParentFrame, StaticFrame>::value>::type,
            typename = typename std::enable_if<
                    std::is_same<typename CConcept::ChildFrame, DynamicFrame>::value>::type
                    >
    explicit FrameReferences(ChildArg&& childFrameId):
            helpers::ParentConcept<ParentFrameId>(),
            helpers::ChildConcept<ChildFrameId>(std::forward<ChildArg>(childFrameId))
    {
    }


    template<
            typename ParentArg,
            typename PConcept = helpers::ParentConcept<DynamicFrame>,
            typename CConcept = helpers::ChildConcept<ChildFrameId>,
            typename = typename std::enable_if<
                    std::is_same<typename PConcept::ParentFrame, DynamicFrame>::value>::type,
            typename = typename std::enable_if<
                    common::IsSpecialization<typename CConcept::ChildFrame, StaticFrame>::value>::type
                    >
    explicit FrameReferences(ParentArg&& parentFrameId, int = 0):
            helpers::ParentConcept<ParentFrameId>(std::forward<ParentArg>(parentFrameId)),
            helpers::ChildConcept<ChildFrameId>()
    {
    }


    template<
            typename ParentArg,
            typename ChildArg,
            typename PConcept = helpers::ParentConcept<DynamicFrame>,
            typename CConcept = helpers::ChildConcept<DynamicFrame>,
            typename = typename std::enable_if<
                    std::is_same<typename PConcept::ParentFrame, DynamicFrame>::value>::type,
            typename = typename std::enable_if<
                    std::is_same<typename CConcept::ChildFrame, DynamicFrame>::value>::type
                    >
    FrameReferences(ParentArg&& parentFrameId, ChildArg&& childFrameId):
            helpers::ParentConcept<ParentFrameId>(std::forward<ParentArg>(parentFrameId)),
            helpers::ChildConcept<ChildFrameId>(std::forward<ChildArg>(childFrameId))
    {
    }
};


} // End of namespace naksh::geometry.
