////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nioc/common/typeTraits.hpp>
#include <string>

namespace nioc::geometry
{
/// @brief  A class representing a static reference frame.
///
///         @usage
///             You may either declare (definition is not required) a new type or use an already
///             existing type that meaningfully represents the frame of reference. Eg. LeftCamera,
///             IMUDriver, etc.
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
  static_assert(
      not(common::isSpecialization<FrameId, StaticFrame>),
      "FrameId cannot be a specialization of the StaticFrame<> template.");

  /// Deleted destructor to prevent runtime instantiation for a static frame
  ~StaticFrame() = delete;

  /// @brief  Accessor for the name of the frame.
  /// @return Pretty formatted name of the underlying frame identity.
  static constexpr const std::string_view& name() noexcept
  {
    return kFrameName;
  }

private:
  /// Name identifying the reference frame. Used at run-time to evaluate
  /// frame compatibility if one of the operands is a dynamic frame.
  static constexpr const auto kFrameName = common::prettyName<FrameId>();
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


} // End of namespace nioc::geometry.
