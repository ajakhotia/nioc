////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frame.hpp"

namespace naksh::geometry
{


/// @brief  A class representing the concept of a parent frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename FrameId>
class ParentFrame
{
public:
    /// Avoid nesting StaticFrame type
    static_assert(not common::IsSpecialization<FrameId, StaticFrame>::value);

    using Parent = StaticFrame<FrameId>;

    virtual ~ParentFrame() = default;
};


/// @brief  A class representing the concept of a child frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename FrameId>
class ChildFrame
{
public:
    /// Avoid nesting StaticFrame type
    static_assert(not common::IsSpecialization<FrameId, StaticFrame>::value);

    using Child = StaticFrame<FrameId>;

    virtual ~ChildFrame() = default;
};


/// @brief  @ParentFrame specialized for the dynamic case.
template<>
class ParentFrame<DynamicFrame>
{
public:
    using Parent = DynamicFrame;

    explicit ParentFrame(Parent parent) noexcept: mParent(std::move(parent)) { }

    explicit ParentFrame(std::string frameId) noexcept:
            ParentFrame(Parent(std::move(frameId)))
    {
    }

    virtual ~ParentFrame() = default;

    [[nodiscard]] const Parent& parentFrame() const noexcept { return mParent; }

private:
    Parent mParent;
};


/// @brief  @ChildFrame specialized for the dynamic case.
template<>
class ChildFrame<DynamicFrame>
{
public:
    using Child = DynamicFrame;

    explicit ChildFrame(Child child) noexcept: mChild(std::move(child)) { }

    explicit ChildFrame(std::string frameId) noexcept:
            ChildFrame(Child(std::move(frameId)))
    {
    }

    virtual ~ChildFrame() = default;

    [[nodiscard]] const Child& childFrame() const noexcept { return mChild; }

private:
    Child mChild;
};


/// @brief
/// @tparam Parent
/// @tparam Child
template<typename ParentFrameId, typename ChildFrameId>
class FrameReferences : public ParentFrame<ParentFrameId>, public ChildFrame<ChildFrameId>
{
public:
    template<typename ...ParentArgs, typename ...ChildArgs>
    explicit FrameReferences(ParentArgs&&... parentArgs, ChildArgs&&... childArgs) :
            ParentFrame<ParentFrameId>(std::forward<ParentArgs>(parentArgs)...),
            ChildFrame<ChildFrameId>(std::forward<ChildArgs>(childArgs)...)
    {
    }
};


} // End of namespace naksh::geometry.
