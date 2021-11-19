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
class ParentConceptTmpl
{
public:
    using ParentFrame = StaticFrame<FrameId>;
    virtual ~ParentConceptTmpl() = default;
};


/// @brief  A class representing the concept of a child frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename FrameId>
class ChildConceptTmpl
{
public:
    using ChildFrame = StaticFrame<FrameId>;
    virtual ~ChildConceptTmpl() = default;
};


/// @brief  ParentConceptTmpl specialized for the dynamic case.
template<>
class ParentConceptTmpl<DynamicFrame>
{
public:
    using ParentFrame = DynamicFrame;

    template<typename ParentFrameArgs>
    explicit ParentConceptTmpl(ParentFrameArgs parentFrameArgs) noexcept: mParentFrame(std::move(parentFrameArgs))
    {
    }


    virtual ~ParentConceptTmpl() = default;

    [[nodiscard]] const ParentFrame& parentFrame() const noexcept
    {
        return mParentFrame;
    }

private:
    ParentFrame mParentFrame;
};


/// @brief  ChildConceptTmpl specialized for the dynamic case.
template<>
class ChildConceptTmpl<DynamicFrame>
{
public:
    using ChildFrame = DynamicFrame;

    template<typename ChildFrameArgs>
    explicit ChildConceptTmpl(ChildFrameArgs childFrameArgs) noexcept: mChildFrame(std::move(childFrameArgs))
    {
    }


    virtual ~ChildConceptTmpl() = default;

    [[nodiscard]] const ChildFrame& childFrame() const noexcept
    {
        return mChildFrame;
    }

private:
    ChildFrame mChildFrame;
};



} // End of namespace naksh::geometry.
