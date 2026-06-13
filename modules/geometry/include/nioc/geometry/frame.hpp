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
/// @brief A reference frame identified by a type at compile time.
///
/// @code
/// class World;
/// using WorldFrame = nioc::geometry::StaticFrame<World>;
/// @endcode
///
/// @tparam FrameId_ Type that names this frame. Cannot itself be a StaticFrame.
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

  /// @brief Returns the frame's name.
  static constexpr const std::string_view& name() noexcept
  {
    return kFrameName;
  }

private:
  static constexpr const auto kFrameName = common::prettyName<FrameId>();
};

/// @brief A reference frame identified by a name string at run time.
class DynamicFrame
{
public:
  /// @param frameId Name of this frame.
  explicit DynamicFrame(std::string frameId) noexcept;

  DynamicFrame(const DynamicFrame&) = default;

  DynamicFrame(DynamicFrame&&) noexcept = default;

  ~DynamicFrame() = default;

  DynamicFrame& operator=(const DynamicFrame&) = default;

  DynamicFrame& operator=(DynamicFrame&&) noexcept = default;

  /// @brief Returns the frame's name.
  [[nodiscard]] const std::string& name() const noexcept;

private:
  std::string mFrameId;
};

/// @return True if both frames have the same name.
bool operator==(const DynamicFrame& lhs, const DynamicFrame& rhs);

/// @return True if the frames have different names.
bool operator!=(const DynamicFrame& lhs, const DynamicFrame& rhs);


} // namespace nioc::geometry
