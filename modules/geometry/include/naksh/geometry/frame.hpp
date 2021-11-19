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
/// @tparam FrameId_   Compile-time identity of the reference frame.
template<typename FrameId_>
class StaticFrame
{
public:
    /// Alias of the underlying identity of the frame.
    using FrameId = FrameId_;

    /// Assert that the FrameId is not a specialization of StaticFrame. This is
    /// done to avoid nesting specialization such as StaticFrame<StaticFrame<...>>.
    static_assert(not(common::IsSpecialization<FrameId, StaticFrame>::value),
                  "FrameId cannot be a specialization of the StaticFrame<> template."  );

    /// Name identifying the reference frame. Used at run-time to evaluate
    /// frame compatibility if one of the operands is a dynamic frame.
    static constexpr const auto kFrameName = common::prettyName<FrameId>();

    /// Deleted destructor to prevent runtime instantiation for a static frame
    ~StaticFrame() = delete;

    /// @brief  Accessor for the name of the frame.
    /// @return Pretty formatted name of the underlying frame identity.
    static constexpr const std::string_view& name() noexcept
    {
        return kFrameName;
    }
};


/// @brief  Class to represent frames with names evaluated at run-time.
class DynamicFrame
{
public:
    /// @brief  Constructor.
    /// @param  frameId   Run-time identity of the reference frame.
    explicit DynamicFrame(std::string frameId) noexcept;

    DynamicFrame(const DynamicFrame&) = default;
    DynamicFrame(DynamicFrame&&) noexcept = default;
    ~DynamicFrame() = default;
    DynamicFrame& operator=(const DynamicFrame&) = default;
    DynamicFrame& operator=(DynamicFrame&&) noexcept = default;

    /// @brief  Accessor for the name of the frame.
    /// @return A string reference that identifies the frame.
    [[nodiscard]] const std::string& name() const noexcept;

private:
    /// Name that represents the identity of the frame at run-time.
    std::string mFrameId;
};


/// @brief  Equality check operator.
/// @param lhs
/// @param rhs
/// @return True if the identity of lhs and rhs are the same. False otherwise.
bool operator==(const DynamicFrame& lhs, const DynamicFrame& rhs);


/// @brief  In-equality check operator.
/// @param lhs
/// @param rhs
/// @return True if the identity of lhs and rhs are not the same. False otherwise.
bool operator!=(const DynamicFrame& lhs, const DynamicFrame& rhs);


template<typename LhsFrame, typename RhsFrame>
class AreSameFrames
{
public:
    static_assert(common::isSpecialization<LhsFrame, StaticFrame>,
            "Provided frame is not a template specialization of StaticFrame<...>");

    static_assert(common::isSpecialization<RhsFrame, StaticFrame>,
                  "Provided frame is not a template specialization of StaticFrame<...>");

    static constexpr bool kValue = LhsFrame::name() == RhsFrame::name();

    static_assert(std::is_same_v<LhsFrame, RhsFrame> == kValue,
                  "Mismatch between the similarity of frame names and frame types");

    [[nodiscard]] static constexpr bool value() noexcept
    {
        return kValue;
    }
};


template<typename LhsFrame>
class AreSameFrames<LhsFrame, DynamicFrame>
{
public:
    static_assert(common::isSpecialization<LhsFrame, StaticFrame>,
                  "Provided frame is not a template specialization of StaticFrame<...>");

    static constexpr bool kValue = true;

    explicit AreSameFrames(const DynamicFrame& rhsFrame) noexcept: mValid(LhsFrame::name() == rhsFrame.name())
    {
    }

    [[nodiscard]] bool value() const noexcept
    {
        return mValid;
    }

private:
    const bool mValid;
};


template<typename RhsFrame>
class AreSameFrames<DynamicFrame, RhsFrame>
{
public:
    static_assert(common::isSpecialization<RhsFrame, StaticFrame>,
                  "Provided frame is not a template specialization of StaticFrame<...>");

    static constexpr bool kValue = true;

    explicit AreSameFrames(const DynamicFrame& lhsFrame) noexcept: mValid(lhsFrame.name() == RhsFrame::name())
    {
    }

    [[nodiscard]] bool value() const noexcept
    {
        return mValid;
    }

private:
    const bool mValid;
};


template<>
class AreSameFrames<DynamicFrame, DynamicFrame>
{
public:
    static constexpr bool kValue = true;

    explicit AreSameFrames(const DynamicFrame& lhsFrame, const DynamicFrame& rhsFrame) noexcept:
            mValid(lhsFrame == rhsFrame)
    {
    }

    [[nodiscard]] bool value() const noexcept
    {
        return mValid;
    }

private:
    const bool mValid;
};

} // End of namespace naksh::geometry.
