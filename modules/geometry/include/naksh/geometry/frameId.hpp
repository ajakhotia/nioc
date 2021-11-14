////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <naksh/common/typetraits.hpp>
#include <boost/type_index/ctti_type_index.hpp>
#include <string>
#include <iostream>

namespace naksh::geometry
{


/// @brief  A class representing a static reference frame.
///
///         @usage
///             You may either declare (definition is not required) a new type or use a an already
///             existing type that is meaningful to represent the frame of reference. Eg. LeftCamera, IMUDriver, etc.
///
///             class World;
///
///             MyCompileTimeCheck<StaticFrame<World>, ... > ...
///
///
/// @tparam FrameType   Type used to statically identify a reference frame.
template<typename FrameType>
class StaticFrame
{
public:
    StaticFrame() = default;
    StaticFrame(const StaticFrame&) = default;
    StaticFrame(StaticFrame&&) noexcept = default;
    ~StaticFrame() = default;
    StaticFrame& operator=(const StaticFrame&) = default;
    StaticFrame& operator=(StaticFrame&&) noexcept = default;

    /// Name identifying the reference frame. Used at run-time to evaluate
    /// frame compatibility if one of the operands is a dynamic frame.
    static constexpr const std::string_view kFrameName =
            boost::typeindex::ctti_type_index::type_id<FrameType>().name();

    /// An accessor for the name of the reference frame.
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
    /// @param  frameName   Run-time identity of the reference frame.
    explicit DynamicFrame(std::string frameName) noexcept;

    DynamicFrame(const DynamicFrame&) = default;
    DynamicFrame(DynamicFrame&&) = default;
    ~DynamicFrame() = default;
    DynamicFrame& operator=(const DynamicFrame&) = default;
    DynamicFrame& operator=(DynamicFrame&&) = default;

    [[nodiscard]] const std::string& name() const noexcept;

private:
    std::string mFrameName;
};


} // End of namespace naksh::geometry.
