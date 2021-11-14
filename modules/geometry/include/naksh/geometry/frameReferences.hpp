////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameId.hpp"

namespace naksh::geometry
{

template<typename ParentFrame_, typename ChildFrame_>
class FrameReferences
{
public:
    using ParentFrame = ParentFrame_;
    using ChildFrame = ChildFrame_;
    FrameReferences() = default;
    ~FrameReferences() = default;

    [[nodiscard]] static constexpr decltype(auto) parentFrameName() noexcept
    {
        return ParentFrame::name();
    }

    [[nodiscard]] static constexpr decltype(auto) childFrameName() noexcept
    {
        return ChildFrame::name();
    }
};


template<typename ParentFrame_>
class FrameReferences<ParentFrame_, DynamicFrame>
{
public:
    using ParentFrame = ParentFrame_;
    using ChildFrame = DynamicFrame;

    explicit FrameReferences(DynamicFrame childFrame): mChildFrame(std::move(childFrame))
    {
    }

    ~FrameReferences() = default;

    [[nodiscard]] static constexpr decltype(auto) parentFrameName() noexcept
    {
        return ParentFrame::name();
    }

    [[nodiscard]] decltype(auto) childFrameName() const noexcept
    {
        return mChildFrame.name();
    }

private:
    ChildFrame mChildFrame;
};


template<typename ChildFrame_>
class FrameReferences<DynamicFrame, ChildFrame_>
{
public:
    using ParentFrame = DynamicFrame;
    using ChildFrame = ChildFrame_;

    explicit FrameReferences(DynamicFrame parentFrame): mParentFrame(std::move(parentFrame))
    {
    }

    ~FrameReferences() = default;

    [[nodiscard]] decltype(auto) parentFrameName() const noexcept
    {
        return mParentFrame.name();
    }

    [[nodiscard]] static constexpr decltype(auto) childFrameName() noexcept
    {
        return ChildFrame::name();
    }

private:
    ParentFrame mParentFrame;
};


template<>
class FrameReferences<DynamicFrame, DynamicFrame>
{
public:
    using ParentFrame = DynamicFrame;
    using ChildFrame = DynamicFrame;

    FrameReferences(DynamicFrame parentFrame, DynamicFrame childFrame):
            mParentFrame(std::move(parentFrame)),
            mChildFrame(std::move(childFrame))
    {
    }

    ~FrameReferences() = default;

    [[nodiscard]] decltype(auto) parentFrameName() const noexcept
    {
        return mParentFrame.name();
    }

    [[nodiscard]] decltype(auto) childFrameName() const noexcept
    {
        return mChildFrame.name();
    }

private:
    ParentFrame mParentFrame;

    ChildFrame mChildFrame;
};


} // End of namespace naksh::geometry.
