////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frame.hpp"
#include <nioc/common/typeTraits.hpp>

namespace nioc::geometry
{
/// @brief Parent frame type wrapper.
///
/// Wraps a frame type to represent parent in a transformation.
///
/// @tparam ParentFrame_ Frame type (StaticFrame or DynamicFrame).
template<typename ParentFrame_>
class ParentConceptTmpl
{
public:
  using ParentFrame = ParentFrame_;

  static_assert(
      common::isSpecialization<ParentFrame, StaticFrame>,
      "Parent frame is not a template specialization of StaticFrame<> class.");

  virtual ~ParentConceptTmpl() = default;

  /// @brief Returns the parent frame as a tuple — empty, since a static frame carries no value.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Child frame type wrapper.
///
/// Wraps a frame type to represent child in a transformation.
///
/// @tparam ChildFrame_ Frame type (StaticFrame or DynamicFrame).
template<typename ChildFrame_>
class ChildConceptTmpl
{
public:
  using ChildFrame = ChildFrame_;

  static_assert(
      common::isSpecialization<ChildFrame, StaticFrame>,
      "Child frame is not a template specialization of StaticFrame<> class.");

  virtual ~ChildConceptTmpl() = default;

  /// @brief Returns the child frame as a tuple — empty, since a static frame carries no value.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Parent frame wrapper for runtime-identified frames.
template<>
class ParentConceptTmpl<DynamicFrame>
{
public:
  using ParentFrame = DynamicFrame;

  /// @brief Stores the runtime parent frame.
  /// @param parentFrameArgs Identifier the parent @ref DynamicFrame is built from.
  template<typename ParentFrameArgs>
  explicit ParentConceptTmpl(ParentFrameArgs parentFrameArgs) noexcept:
      mParentFrame(std::move(parentFrameArgs))
  {
  }

  virtual ~ParentConceptTmpl() = default;

  /// @brief Returns the stored parent frame.
  [[nodiscard]] constexpr const ParentFrame& parentFrame() const noexcept
  {
    return mParentFrame;
  }

  /// @brief Returns the parent frame as a single-element tuple holding a reference to it.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mParentFrame));
  }

private:
  ParentFrame mParentFrame;
};

/// @brief Child frame wrapper for runtime-identified frames.
template<>
class ChildConceptTmpl<DynamicFrame>
{
public:
  using ChildFrame = DynamicFrame;

  /// @brief Stores the runtime child frame.
  /// @param childFrameArgs Identifier the child @ref DynamicFrame is built from.
  template<typename ChildFrameArgs>
  explicit ChildConceptTmpl(ChildFrameArgs childFrameArgs) noexcept:
      mChildFrame(std::move(childFrameArgs))
  {
  }

  virtual ~ChildConceptTmpl() = default;

  /// @brief Returns the stored child frame.
  [[nodiscard]] constexpr const ChildFrame& childFrame() const noexcept
  {
    return mChildFrame;
  }

  /// @brief Returns the child frame as a single-element tuple holding a reference to it.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mChildFrame));
  }

private:
  ChildFrame mChildFrame;
};


} // namespace nioc::geometry
