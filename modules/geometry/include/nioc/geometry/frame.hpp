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
/// @brief A reference frame with compile-time identity.
///
/// A type names the frame.
///
/// @code
/// class World;
/// using WorldFrame = nioc::geometry::StaticFrame<World>;
/// @endcode
///
/// @tparam FrameId_ Type that identifies this frame.
template<typename FrameId_>
class StaticFrame
{
public:
  using FrameId = FrameId_;

  static_assert(
      not(common::isSpecialization<FrameId, StaticFrame>),
      "FrameId cannot be a StaticFrame specialization.");

  ~StaticFrame() = delete;

  /// @brief Returns the frame's name.
  static constexpr const std::string_view& name() noexcept
  {
    return kFrameName;
  }

private:
  static constexpr const auto kFrameName = common::prettyName<FrameId>();
};

/// @brief A reference frame with runtime identity.
class DynamicFrame
{
public:
  /// @brief Constructs a dynamic frame.
  /// @param frameId Name identifying this frame.
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

/// @brief Checks if two frames are the same.
/// @return True if frames have the same identity.
bool operator==(const DynamicFrame& lhs, const DynamicFrame& rhs);

/// @brief Checks if two frames are different.
/// @return True if frames have different identities.
bool operator!=(const DynamicFrame& lhs, const DynamicFrame& rhs);


} // namespace nioc::geometry
