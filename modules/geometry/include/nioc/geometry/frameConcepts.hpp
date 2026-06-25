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
/// @brief Base class that adds the parent-frame side of a frame-bearing type's interface.
///
/// Mix this in to give a type a parent frame plus a uniform `parentFrameAsTuple()` accessor that
/// frame-composition algorithms splice together. `FrameReferences` derives from it; you rarely name
/// it directly. This primary template is for a parent frame fixed at compile time: the frame's
/// identity lives in the type, so an instance stores nothing. See the `DynamicFrame` specialization
/// for a parent frame chosen at run time.
///
/// @tparam ParentFrame_ Must be a `StaticFrame<>` specialization (enforced by static assertion).
/// Pass `DynamicFrame` to select the run-time specialization instead.
///
/// @see ParentConceptTmpl<DynamicFrame>, ChildConceptTmpl, FrameReferences, composeFrameReferences
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

  /// @brief Return an empty tuple; the static parent frame carries no run-time value to splice.
  ///
  /// Provided so composition algorithms can treat static and dynamic frames uniformly via
  /// `std::tuple_cat`.
  ///
  /// @return An empty `std::tuple<>`.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Base class that adds the child-frame side of a frame-bearing type's interface.
///
/// Child-frame counterpart of `ParentConceptTmpl`. Mix it in to give a type a child frame plus a
/// uniform `childFrameAsTuple()` accessor for frame-composition algorithms. This primary template
/// is for a child frame fixed at compile time and stores nothing. See the `DynamicFrame`
/// specialization for a child frame chosen at run time.
///
/// @tparam ChildFrame_ Must be a `StaticFrame<>` specialization (enforced by static assertion).
/// Pass `DynamicFrame` to select the run-time specialization instead.
///
/// @see ChildConceptTmpl<DynamicFrame>, ParentConceptTmpl, FrameReferences, composeFrameReferences
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

  /// @brief Return an empty tuple; the static child frame carries no run-time value to splice.
  ///
  /// Provided so composition algorithms can treat static and dynamic frames uniformly via
  /// `std::tuple_cat`.
  ///
  /// @return An empty `std::tuple<>`.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple();
  }
};

/// @brief Parent-frame mixin for a parent frame whose identity is a run-time string.
///
/// Specialization of `ParentConceptTmpl` that stores a `DynamicFrame`. Use it when the parent frame
/// is known only at run time. Adds `parentFrame()` to read the stored value, and
/// `parentFrameAsTuple()` now yields that value instead of an empty tuple.
///
/// @see ParentConceptTmpl, DynamicFrame, FrameReferences
template<>
class ParentConceptTmpl<DynamicFrame>
{
public:
  using ParentFrame = DynamicFrame;

  /// @brief Construct the stored parent frame from a value that builds a `DynamicFrame`.
  ///
  /// @tparam ParentFrameArgs Any type accepted by `DynamicFrame`'s constructor, e.g. a
  /// `std::string` frame id.
  ///
  /// @param parentFrameArgs Taken by value and moved into the stored frame.
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

  /// @brief Return the stored parent frame.
  ///
  /// @return Reference valid for this object's lifetime.
  [[nodiscard]] constexpr const ParentFrame& parentFrame() const noexcept
  {
    return mParentFrame;
  }

  /// @brief Return the parent frame as a one-element tuple holding `std::cref(parentFrame())`.
  ///
  /// Lets composition algorithms splice the frame uniformly via `std::tuple_cat`. The tuple borrows
  /// a reference: do not let it outlive this object.
  ///
  /// @return A `std::tuple` of one `std::reference_wrapper<const DynamicFrame>`.
  [[nodiscard]] constexpr decltype(auto) parentFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mParentFrame));
  }

private:
  /// The run-time parent frame, set at construction and returned by `parentFrame()`.
  ParentFrame mParentFrame;
};

/// @brief Child-frame mixin for a child frame whose identity is a run-time string.
///
/// Specialization of `ChildConceptTmpl` that stores a `DynamicFrame`. Use it when the child frame
/// is known only at run time. Adds `childFrame()` to read the stored value, and
/// `childFrameAsTuple()` now yields that value instead of an empty tuple.
///
/// @see ChildConceptTmpl, DynamicFrame, FrameReferences
template<>
class ChildConceptTmpl<DynamicFrame>
{
public:
  using ChildFrame = DynamicFrame;

  /// @brief Construct the stored child frame from a value that builds a `DynamicFrame`.
  ///
  /// @tparam ChildFrameArgs Any type accepted by `DynamicFrame`'s constructor, e.g. a
  /// `std::string` frame id.
  ///
  /// @param childFrameArgs Taken by value and moved into the stored frame.
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

  /// @brief Return the stored child frame.
  ///
  /// @return Reference valid for this object's lifetime.
  [[nodiscard]] constexpr const ChildFrame& childFrame() const noexcept
  {
    return mChildFrame;
  }

  /// @brief Return the child frame as a one-element tuple holding `std::cref(childFrame())`.
  ///
  /// Lets composition algorithms splice the frame uniformly via `std::tuple_cat`. The tuple borrows
  /// a reference: do not let it outlive this object.
  ///
  /// @return A `std::tuple` of one `std::reference_wrapper<const DynamicFrame>`.
  [[nodiscard]] constexpr decltype(auto) childFrameAsTuple() const noexcept
  {
    return std::make_tuple(std::cref(mChildFrame));
  }

private:
  /// The run-time child frame, set at construction and returned by `childFrame()`.
  ChildFrame mChildFrame;
};


} // namespace nioc::geometry
