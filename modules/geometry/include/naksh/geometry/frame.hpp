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
class FramesEqual
{
public:
    static_assert(common::isSpecialization<LhsFrame, StaticFrame> or std::is_same_v<LhsFrame, DynamicFrame>,
            "Provided LhsFrame is neither a specialization of StaticFrame<...> nor is a DynamicFrame");

    static_assert(common::isSpecialization<RhsFrame, StaticFrame> or std::is_same_v<RhsFrame, DynamicFrame>,
            "Provided RhsFrame is neither a specialization of StaticFrame<...> nor is a DynamicFrame");

    static constexpr bool kValue =
            std::is_same_v<LhsFrame, DynamicFrame> or
            std::is_same_v<RhsFrame, DynamicFrame> or
            std::is_same_v<LhsFrame, RhsFrame>;

    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            common::isSpecialization<Lhs, StaticFrame> and common::isSpecialization<Rhs, StaticFrame>>>
    FramesEqual() noexcept = delete;


    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            common::isSpecialization<Lhs, StaticFrame> and std::is_same_v<Rhs, DynamicFrame>>>
    explicit FramesEqual(const Rhs& rhsFrame) noexcept: mValue(Lhs::name() == rhsFrame.name())
    {
    }


    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            std::is_same_v<Lhs, DynamicFrame> and common::isSpecialization<Rhs, StaticFrame>>>
    explicit FramesEqual(const Lhs& lhsFrame, int = 0) noexcept: mValue(lhsFrame.name() == Rhs::name())
    {
    }


    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            std::is_same_v<Lhs, DynamicFrame> and std::is_same_v<Rhs, DynamicFrame>>>
    FramesEqual(const Lhs& lhsFrame, const Rhs& rhsFrame) noexcept: mValue(lhsFrame.name() == rhsFrame.name())
    {
    }


    ~FramesEqual() = default;


    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            common::isSpecialization<Lhs, StaticFrame> and common::isSpecialization<Rhs, StaticFrame>>>
    [[nodiscard]] static constexpr bool value() noexcept
    {
        return kValue;
    }


    template<
        typename Lhs = LhsFrame,
        typename Rhs = RhsFrame,
        typename = typename std::enable_if_t<
            std::is_same_v<Lhs, DynamicFrame> or std::is_same_v<Rhs, DynamicFrame>>>
    [[nodiscard]] bool value(int = 0) const noexcept
    {
        return mValue;
    }
private:
    bool mValue;
};


} // End of namespace naksh::geometry.
