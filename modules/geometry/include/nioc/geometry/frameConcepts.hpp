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
/// @brief Holds the parent frame of a transformation.
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

  ParentConceptTmpl() = default;

  ParentConceptTmpl(const ParentConceptTmpl&) = default;

  ParentConceptTmpl(ParentConceptTmpl&&) noexcept = default;

  ParentConceptTmpl& operator=(const ParentConceptTmpl&) = default;

  ParentConceptTmpl& operator=(ParentConceptTmpl&&) noexcept = default;

  virtual ~ParentConceptTmpl() = default;

  /// @brief Returns an empty tuple. A static frame carries no value.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Holds the child frame of a transformation.
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

  ChildConceptTmpl() = default;

  ChildConceptTmpl(const ChildConceptTmpl&) = default;

  ChildConceptTmpl(ChildConceptTmpl&&) noexcept = default;

  ChildConceptTmpl& operator=(const ChildConceptTmpl&) = default;

  ChildConceptTmpl& operator=(ChildConceptTmpl&&) noexcept = default;

  virtual ~ChildConceptTmpl() = default;

  /// @brief Returns an empty tuple. A static frame carries no value.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Holds a runtime-identified parent frame.
template<>
class ParentConceptTmpl<DynamicFrame>
{
public:
  using ParentFrame = DynamicFrame;

  /// @brief Builds and stores the parent frame from a runtime identifier.
  /// @param parentFrameArgs Identifier the parent @ref DynamicFrame is built from.
  template<typename ParentFrameArgs>
  explicit ParentConceptTmpl(ParentFrameArgs parentFrameArgs) noexcept:
    mParentFrame(std::move(parentFrameArgs))
  {
  }

  ParentConceptTmpl(const ParentConceptTmpl&) = default;

  ParentConceptTmpl(ParentConceptTmpl&&) noexcept = default;

  ParentConceptTmpl& operator=(const ParentConceptTmpl&) = default;

  ParentConceptTmpl& operator=(ParentConceptTmpl&&) noexcept = default;

  virtual ~ParentConceptTmpl() = default;

  /// @brief Returns the stored parent frame.
  [[nodiscard]] constexpr const ParentFrame& parentFrame() const noexcept
  {
    return mParentFrame;
  }

  /// @brief Returns a single-element tuple holding a reference to the parent frame.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mParentFrame));
  }

private:
  ParentFrame mParentFrame;
};

/// @brief Holds a runtime-identified child frame.
template<>
class ChildConceptTmpl<DynamicFrame>
{
public:
  using ChildFrame = DynamicFrame;

  /// @brief Builds and stores the child frame from a runtime identifier.
  /// @param childFrameArgs Identifier the child @ref DynamicFrame is built from.
  template<typename ChildFrameArgs>
  explicit ChildConceptTmpl(ChildFrameArgs childFrameArgs) noexcept:
    mChildFrame(std::move(childFrameArgs))
  {
  }

  ChildConceptTmpl(const ChildConceptTmpl&) = default;

  ChildConceptTmpl(ChildConceptTmpl&&) noexcept = default;

  ChildConceptTmpl& operator=(const ChildConceptTmpl&) = default;

  ChildConceptTmpl& operator=(ChildConceptTmpl&&) noexcept = default;

  virtual ~ChildConceptTmpl() = default;

  /// @brief Returns the stored child frame.
  [[nodiscard]] constexpr const ChildFrame& childFrame() const noexcept
  {
    return mChildFrame;
  }

  /// @brief Returns a single-element tuple holding a reference to the child frame.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mChildFrame));
  }

private:
  ChildFrame mChildFrame;
};


} // namespace nioc::geometry
