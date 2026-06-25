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
/// @brief A coordinate frame whose identity is fixed at compile time by a tag type.
///
/// Each distinct @p FrameId_ tag names one frame. The frame's name is derived from the tag's
/// type name, so two `StaticFrame` specializations name the same frame only if their tags are the
/// same type. This is a pure compile-time vocabulary type: it has no objects, no state, and no
/// runtime cost. Use it as a template argument (for example to `FrameReferences`) and call its
/// static `name()`.
///
/// Example:
///
///     struct WorldTag {};
///     using World = StaticFrame<WorldTag>;
///     std::string_view n = World::name();  // e.g. "WorldTag"
///
/// @tparam FrameId_ The tag type identifying the frame. Must be named explicitly and must not
/// itself be a `StaticFrame` specialization; violating this fails a static assertion.
///
/// @see DynamicFrame, FrameReferences
template<typename FrameId_>
class StaticFrame
{
public:
  using FrameId = FrameId_;

  static_assert(
      not(common::isSpecialization<FrameId, StaticFrame>),
      "FrameId cannot be a StaticFrame specialization.");

  StaticFrame() = delete;

  StaticFrame(const StaticFrame&) = delete;

  StaticFrame(StaticFrame&&) = delete;

  StaticFrame& operator=(const StaticFrame&) = delete;

  StaticFrame& operator=(StaticFrame&&) = delete;

  ~StaticFrame() = delete;

  /// @brief Return the frame's name.
  ///
  /// The returned view points at static storage that lives for the whole program. The exact
  /// spelling is compiler-dependent, so use it for diagnostics, not as a stable identifier.
  static constexpr const std::string_view& name() noexcept
  {
    return kFrameName;
  }

private:
  /// The frame's name, derived once at compile time from the tag type's spelling. Backs name().
  static constexpr const auto kFrameName = common::prettyName<FrameId>();
};

/// @brief A coordinate frame whose name is chosen at run time by a string.
///
/// Use this when frames are not known until run time. The name is set at construction and never
/// changes afterward. This is the value-semantic counterpart to `StaticFrame`: it holds state, is
/// freely copyable and movable, and compares equal to another frame when their names match.
///
/// Example:
///
///     DynamicFrame world{"world"};
///     DynamicFrame sensor{"sensor"};
///     bool same = (world == sensor);  // false
///
/// @see StaticFrame, operator==, operator!=
class DynamicFrame
{
public:
  /// @brief Construct a frame with the given name.
  ///
  /// @param frameId The frame's name. Moved into the frame.
  explicit DynamicFrame(std::string frameId) noexcept;

  DynamicFrame(const DynamicFrame&) = default;

  DynamicFrame(DynamicFrame&&) noexcept = default;

  ~DynamicFrame() = default;

  DynamicFrame& operator=(const DynamicFrame&) = default;

  DynamicFrame& operator=(DynamicFrame&&) noexcept = default;

  /// @brief Return the frame's name.
  ///
  /// The reference stays valid until the frame is assigned to, moved from, or destroyed.
  [[nodiscard]] const std::string& name() const noexcept;

private:
  /// The frame's name, set at construction and never changed afterward. Returned by name().
  std::string mFrameId;
};

/// @brief Return true when both frames have the same name.
bool operator==(const DynamicFrame& lhs, const DynamicFrame& rhs);

/// @brief Return true when the frames have different names.
bool operator!=(const DynamicFrame& lhs, const DynamicFrame& rhs);


} // namespace nioc::geometry
