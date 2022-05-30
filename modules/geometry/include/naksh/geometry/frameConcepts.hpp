////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frame.hpp"
#include <naksh/common/typeTraits.hpp>

namespace naksh::geometry
{
/// @brief  A class representing the concept of a parent frame.
/// @tparam ParentFrame_ Compile time identity of the frame.
template<typename ParentFrame_>
class ParentConceptTmpl
{
public:
    using ParentFrame = ParentFrame_;

    static_assert(common::isSpecialization<ParentFrame, StaticFrame>,
                  "Parent frame is not a template specialization of StaticFrame<> class.");

    virtual ~ParentConceptTmpl() = default;

    [[nodiscard]] decltype(auto) parentFrameAsTuple() const noexcept
    {
        return std::make_tuple();
    }
};


/// @brief  A class representing the concept of a child frame.
/// @tparam ChildFrame_ Compile time identity of the frame.
template<typename ChildFrame_>
class ChildConceptTmpl
{
public:
    using ChildFrame = ChildFrame_;

    static_assert(common::isSpecialization<ChildFrame, StaticFrame>,
                  "Child frame is not a template specialization of StaticFrame<> class.");

    virtual ~ChildConceptTmpl() = default;

    [[nodiscard]] decltype(auto) childFrameAsTuple() const noexcept
    {
        return std::make_tuple();
    }
};


/// @brief  ParentConceptTmpl specialized for the dynamic case.
template<>
class ParentConceptTmpl<DynamicFrame>
{
public:
    using ParentFrame = DynamicFrame;

    template<typename ParentFrameArgs>
    explicit ParentConceptTmpl(ParentFrameArgs parentFrameArgs) noexcept:
        mParentFrame(std::move(parentFrameArgs))
    {
    }

    virtual ~ParentConceptTmpl() = default;

    [[nodiscard]] const ParentFrame& parentFrame() const noexcept
    {
        return mParentFrame;
    }

    [[nodiscard]] decltype(auto) parentFrameAsTuple() const noexcept
    {
        return std::make_tuple(std::cref(mParentFrame));
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
    explicit ChildConceptTmpl(ChildFrameArgs childFrameArgs) noexcept:
        mChildFrame(std::move(childFrameArgs))
    {
    }


    virtual ~ChildConceptTmpl() = default;

    [[nodiscard]] const ChildFrame& childFrame() const noexcept
    {
        return mChildFrame;
    }

    [[nodiscard]] decltype(auto) childFrameAsTuple() const noexcept
    {
        return std::make_tuple(std::cref(mChildFrame));
    }

private:
    ChildFrame mChildFrame;
};


} // End of namespace naksh::geometry.
