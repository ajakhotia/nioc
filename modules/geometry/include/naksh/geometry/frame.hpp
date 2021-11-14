////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <naksh/common/typeTraits.hpp>
#include <string>

namespace naksh::geometry
{


/// @brief  A class representing a static reference frame.
///
///         @usage
///             You may either declare (definition is not required) a new type or use an already
///             existing type that meaningfully represents the frame of reference. Eg. LeftCamera, IMUDriver, etc.
///
///             class World;
///
///             MyCompileTimeCheck<StaticFrame<World>, ... > ...
///
/// @tparam FrameId_   Type used to statically identify a reference frame.
template<typename FrameId_>
class StaticFrame
{
public:
    /// Alias of the underlying identity of the frame.
    using FrameId = FrameId_;

    /// Assert that the FrameId is not a specialization of StaticFrame. This is
    /// done to avoid nesting specialization such as StaticFrame<StaticFrame<...>>.
    static_assert(not(common::IsSpecialization<FrameId, StaticFrame>::value));

    /// Name identifying the reference frame. Used at run-time to evaluate
    /// frame compatibility if one of the operands is a dynamic frame.
    static constexpr const auto kFrameName = common::prettyName<FrameId>();

    /// @brief  Accessor for the name of the frame.
    /// @return Pretty formatted name of the underlying frame identity.
    static constexpr const std::string_view& name() noexcept
    {
        return kFrameName;
    }
};


/// @brief  Class to represent frames with names evaluated at run-time.
/// @tparam FrameId_    Underlying representation to used to identify frames at run-time.
template<typename FrameId_ = std::string>
class DynamicFrame
{
public:
    using FrameId = FrameId_;

    /// @brief  Constructor.
    /// @param  frameId   Run-time identity of the reference frame.
    template<typename ...Args>
    explicit DynamicFrame(Args&& ...args) noexcept : mFrameId(std::forward<Args>(args)...)
    {
    }

    DynamicFrame(const DynamicFrame&) = default;
    DynamicFrame(DynamicFrame&&) noexcept = default;
    ~DynamicFrame() = default;
    DynamicFrame& operator=(const DynamicFrame&) = default;
    DynamicFrame& operator=(DynamicFrame&&) noexcept = default;

    /// @brief  Accessor for the name of the frame.
    /// @return A string reference that identifies the frame.
    [[nodiscard]] const FrameId& name() const noexcept
    {
        return mFrameId;
    }

private:
    /// Name that represents the identity of the frame at run-time.
    FrameId mFrameId;
};


/// @brief  A class representing the concept of a parent frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename Parent_>
class ParentFrame
{
public:
    using Parent = Parent_;

    virtual ~ParentFrame() = default;
};


/// @brief  A class representing the concept of a child frame.
/// @tparam FrameId Compile time identity of the frame.
template<typename Child_>
class ChildFrame
{
public:
    using Child = Child_;

    virtual ~ChildFrame() = default;
};


/// @brief  @ParentFrame specialized for the dynamic case.
/// @tparam FrameId Underlying representation for the identity of the frame.
template<typename FrameId>
class ParentFrame<DynamicFrame<FrameId>>
{
public:
    using Parent = DynamicFrame<FrameId>;

    explicit ParentFrame(Parent parent) noexcept: mParent(std::move(parent)) { };

    explicit ParentFrame(FrameId frameId) noexcept: ParentFrame(Parent(frameId)) { };

    virtual ~ParentFrame() = default;

    [[nodiscard]] const Parent& parentFrame() const noexcept { return mParent; };

private:
    Parent mParent;
};


/// @brief  @ChildFrame specialized for the dynamic case.
/// @tparam FrameId Underlying representation for the identity of the frame.
template<typename FrameId>
class ChildFrame<DynamicFrame<FrameId>>
{
public:
    using Child = DynamicFrame<FrameId>;

    explicit ChildFrame(Child child) noexcept: mChild(std::move(child)) { };

    explicit ChildFrame(FrameId frameId) noexcept: ChildFrame(Child(std::move(frameId))) { };

    virtual ~ChildFrame() = default;

    [[nodiscard]] const Child& childFrame() const noexcept { return mChild; };

private:
    Child mChild;
};


/// @brief
/// @tparam FrameId
/// @param lhs
/// @param rhs
/// @return
template<typename LhsFrameId, typename RhsFrameId>
bool operator==(const DynamicFrame<LhsFrameId>& lhs, const DynamicFrame<RhsFrameId>& rhs)
{
    return lhs.name() == rhs.name();
}


/// @brief
/// @tparam FrameId
/// @param lhs
/// @param rhs
/// @return
template<typename LhsFrameId, typename RhsFrameId>
bool operator!=(const DynamicFrame<LhsFrameId>& lhs, const DynamicFrame<RhsFrameId>& rhs)
{
    return lhs.name() != rhs.name();
}


} // End of namespace naksh::geometry.
