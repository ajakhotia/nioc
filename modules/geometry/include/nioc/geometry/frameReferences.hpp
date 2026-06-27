////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameConcepts.hpp"
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <string>
#include <type_traits>
#include <utility>

namespace nioc::geometry
{
namespace detail
{
/// @brief Coerce a frame id into the type a frame end stores, turning a char array into a
/// `std::string` and otherwise forwarding the value through unchanged.
///
/// A char array (a string literal) decays to a `std::string` so the frame end owns its name; any
/// other argument is perfectly forwarded so no copy is forced.
///
/// @tparam FrameArg The deduced argument type; a char array triggers the `std::string` conversion.
///
/// @param frameId The frame name to normalize.
///
/// @return A `std::string` when @p frameId is a char array, otherwise @p frameId forwarded as-is.
template<typename FrameArg>
auto normalizeFrameId(FrameArg&& frameId)
{
  if constexpr(std::is_array_v<std::remove_reference_t<FrameArg>>)
  {
    return std::string(static_cast<const char*>(std::forward<FrameArg>(frameId)));
  }
  else
  {
    return std::forward<FrameArg>(frameId);
  }
}
} // namespace detail

/// @brief Names the parent (source) and child (target) frames that a transform relates, without
/// storing the transform itself.
///
/// Each frame end is either a `StaticFrame<>` specialization (identity fixed in the type, no
/// runtime data) or `DynamicFrame` (identity is a runtime string). The mix you pick selects which
/// constructor is callable: pass a runtime id for each dynamic end, and nothing for each static
/// end.
///
/// Example:
///
///     struct WorldTag {}; struct BodyTag {};
///     using World = StaticFrame<WorldTag>;
///     using Body = StaticFrame<BodyTag>;
///
///     FrameReferences<World, Body> a;                 // both static
///     FrameReferences<World, DynamicFrame> b("lidar"); // static parent, dynamic child
///     FrameReferences<DynamicFrame, DynamicFrame> c("map", "odom"); // both dynamic
///
/// @tparam ParentFrame_ A `StaticFrame<>` specialization or `DynamicFrame`.
///
/// @tparam ChildFrame_ A `StaticFrame<>` specialization or `DynamicFrame`.
///
/// @tparam ParentConcept Defaulted base that carries the parent side; do not override.
///
/// @tparam ChildConcept Defaulted base that carries the child side; do not override.
///
/// @see composeFrameReferences, invertFrameReferences, StaticFrame, DynamicFrame
template<
    typename ParentFrame_,
    typename ChildFrame_,
    typename ParentConcept = ParentConceptTmpl<ParentFrame_>,
    typename ChildConcept = ChildConceptTmpl<ChildFrame_>>
// NOLINTNEXTLINE(misc-multiple-inheritance)
class FrameReferences: public ParentConcept, public ChildConcept
{
public:
  /// @brief This same type with the concept bases left defaulted; lets the constructors below name
  /// the resolved `ParentFrame` and `ChildFrame` aliases in their constraints.
  using SelfType = FrameReferences<ParentFrame_, ChildFrame_>;

  /// @brief Default-construct when both frames are static.
  ///
  /// Available only when parent and child are both `StaticFrame<>` specializations.
  template<typename ParentFrame = SelfType::ParentFrame, typename ChildFrame = SelfType::ChildFrame>
  FrameReferences() noexcept
    requires(common::isSpecialization<ParentFrame, StaticFrame> and
             common::isSpecialization<ChildFrame, StaticFrame>)
    : ParentConcept(), ChildConcept()
  {
  }

  /// @brief Construct with a static parent and a dynamic child named by @p childId.
  ///
  /// Available only when the parent is `StaticFrame<>` and the child is `DynamicFrame`.
  ///
  /// @param childId The child frame's name. Any value a `DynamicFrame` accepts, or a char array.
  template<
      typename ChildConceptArgs,
      typename ParentFrame = SelfType::ParentFrame,
      typename ChildFrame = SelfType::ChildFrame>
  [[maybe_unused]] explicit FrameReferences(ChildConceptArgs&& childId) noexcept
    requires(common::isSpecialization<ParentFrame, StaticFrame> and
             std::is_same_v<ChildFrame, DynamicFrame>)
    :
    ParentConcept(),
    ChildConcept(detail::normalizeFrameId(std::forward<ChildConceptArgs>(childId)))
  {
  }

  /// @brief Construct with a dynamic parent named by @p parentId and a static child.
  ///
  /// Available only when the parent is `DynamicFrame` and the child is `StaticFrame<>`. The unnamed
  /// trailing `int` is a disambiguator separating this overload from the
  /// dynamic-parent/dynamic-child one; leave it defaulted.
  ///
  /// @param parentId The parent frame's name. Any value a `DynamicFrame` accepts, or a char array.
  template<
      typename ParentConceptArgs,
      typename ParentFrame = SelfType::ParentFrame,
      typename ChildFrame = SelfType::ChildFrame>
  [[maybe_unused]] explicit FrameReferences(
      ParentConceptArgs&& parentId,
      int /*unused*/ = 0) noexcept
    requires(std::is_same_v<ParentFrame, DynamicFrame> and
             common::isSpecialization<ChildFrame, StaticFrame>)
    :
    ParentConcept(detail::normalizeFrameId(std::forward<ParentConceptArgs>(parentId))),
    ChildConcept()
  {
  }

  /// @brief Construct with both frames dynamic, named by @p parentId and @p childId.
  ///
  /// Available only when both parent and child are `DynamicFrame`.
  ///
  /// @param parentId The parent frame's name. Any value a `DynamicFrame` accepts, or a char array.
  ///
  /// @param childId The child frame's name. Any value a `DynamicFrame` accepts, or a char array.
  template<
      typename ParentConceptArgs,
      typename ChildConceptArgs,
      typename ParentFrame = SelfType::ParentFrame,
      typename ChildFrame = SelfType::ChildFrame>
  [[maybe_unused]] FrameReferences(
      ParentConceptArgs&& parentId,
      ChildConceptArgs&& childId) noexcept
    requires(std::is_same_v<ParentFrame, DynamicFrame> and std::is_same_v<ChildFrame, DynamicFrame>)
    :
    ParentConcept(detail::normalizeFrameId(std::forward<ParentConceptArgs>(parentId))),
    ChildConcept(detail::normalizeFrameId(std::forward<ChildConceptArgs>(childId)))
  {
  }

  FrameReferences(const FrameReferences&) = default;

  FrameReferences(FrameReferences&&) noexcept = default;

  FrameReferences& operator=(const FrameReferences&) = default;

  FrameReferences& operator=(FrameReferences&&) noexcept = default;

  ~FrameReferences() override = default;
};

/// @brief Thrown when frame references are composed across a mismatched inner frame.
///
/// Raised by `composeFrameReferences` (via `assertFrameEqual`) when at least one inner frame is
/// dynamic and the runtime names do not match. Inherits `std::runtime_error`'s constructors.
///
/// @see composeFrameReferences
class FrameCompositionException final: public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

namespace helpers
{
/// @brief Build the message used when composition finds a mismatched inner frame.
///
/// Intended for the failure path of `assertFrameEqual`.
///
/// @param lhsFrameName The left reference's child-frame name.
///
/// @param rhsFrameName The right reference's parent-frame name. Must differ from @p lhsFrameName;
/// equal names trip a debug-build assertion.
///
/// @return A human-readable diagnostic naming both frames.
std::string frameCompositionErrorMessage(
    const std::string& lhsFrameName,
    const std::string& rhsFrameName);

/// @brief Statically assert two static frames name the same frame; a no-op when they do.
///
/// The constraint requires `LhsFrame::name() == RhsFrame::name()`, so a mismatch removes this
/// overload and surfaces as a compile error at the call site rather than throwing.
///
/// @tparam LhsFrame A `StaticFrame<>` specialization; must be named explicitly.
///
/// @tparam RhsFrame A `StaticFrame<>` specialization; must be named explicitly.
template<typename LhsFrame, typename RhsFrame>
constexpr void assertFrameEqual() noexcept
  requires(
      common::isSpecialization<LhsFrame, StaticFrame> and
      common::isSpecialization<RhsFrame, StaticFrame> and
      LhsFrame::name() == RhsFrame::name())
{
}

/// @brief Assert a static `LhsFrame` names the same frame as the runtime @p rhsFrame; throw if not.
///
/// @tparam LhsFrame A `StaticFrame<>` specialization; must be named explicitly (not deducible).
///
/// @param rhsFrame The runtime `DynamicFrame` to compare against.
///
/// @throws FrameCompositionException When the names differ.
template<typename LhsFrame, typename RhsFrame>
inline void assertFrameEqual(const RhsFrame& rhsFrame)
  requires(
      common::isSpecialization<LhsFrame, StaticFrame> and std::is_same_v<RhsFrame, DynamicFrame>)
{
  if(LhsFrame::name() != rhsFrame.name())
  {
    common::throwException<FrameCompositionException>(
        "{}",
        frameCompositionErrorMessage(std::string(LhsFrame::name()), rhsFrame.name()));
  }
}

/// @brief Assert the runtime @p lhsFrame names the same frame as static `RhsFrame`; throw if not.
///
/// @tparam RhsFrame A `StaticFrame<>` specialization; must be named explicitly (not deducible).
///
/// @param lhsFrame The runtime `DynamicFrame` to compare against.
///
/// @throws FrameCompositionException When the names differ.
template<typename LhsFrame, typename RhsFrame>
inline void assertFrameEqual(const LhsFrame& lhsFrame)
  requires(
      std::is_same_v<LhsFrame, DynamicFrame> and common::isSpecialization<RhsFrame, StaticFrame>)
{
  if(lhsFrame.name() != RhsFrame::name())
  {
    common::throwException<FrameCompositionException>(
        "{}",
        frameCompositionErrorMessage(lhsFrame.name(), std::string(RhsFrame::name())));
  }
}

/// @brief Assert two runtime frames name the same frame; throw if not.
///
/// Both frames must be `DynamicFrame`.
///
/// @throws FrameCompositionException When the names differ.
template<typename LhsFrame, typename RhsFrame>
inline void assertFrameEqual(const LhsFrame& lhsFrame, const RhsFrame& rhsFrame)
  requires(std::is_same_v<LhsFrame, DynamicFrame> and std::is_same_v<RhsFrame, DynamicFrame>)
{
  if(lhsFrame.name() != rhsFrame.name())
  {
    common::throwException<FrameCompositionException>(
        "{}",
        frameCompositionErrorMessage(lhsFrame.name(), rhsFrame.name()));
  }
}


} // namespace helpers

/// @brief Chain two frame references into one relating the left parent to the right child.
///
/// Mirrors composing transform @p lhsFrameReferences (parent -> child) with @p rhsFrameReferences
/// (parent -> child). The shared inner frame -- the left reference's child and the right
/// reference's parent -- must match. When both inner frames are static the match is a compile-time
/// constraint; when either is dynamic the names are compared at runtime.
///
/// @throws FrameCompositionException When an inner frame is dynamic and the names differ.
///
/// @see invertFrameReferences
template<typename LhsFrameReferences, typename RhsFrameReferences>
constexpr FrameReferences<
    typename LhsFrameReferences::ParentFrame,
    typename RhsFrameReferences::ChildFrame>
composeFrameReferences(
    const LhsFrameReferences& lhsFrameReferences,
    const RhsFrameReferences& rhsFrameReferences)
{
  using ResultFrameReference = FrameReferences<
      typename LhsFrameReferences::ParentFrame,
      typename RhsFrameReferences::ChildFrame>;

  std::apply(
      helpers::assertFrameEqual<
          typename LhsFrameReferences::ChildFrame,
          typename RhsFrameReferences::ParentFrame>,
      std::tuple_cat(
          lhsFrameReferences.childFrameAsTuple(),
          rhsFrameReferences.parentFrameAsTuple()));

  return std::make_from_tuple<ResultFrameReference>(std::tuple_cat(
      lhsFrameReferences.parentFrameAsTuple(),
      rhsFrameReferences.childFrameAsTuple()));
}

/// @brief Swap parent and child, yielding the reference for the inverse transform.
///
/// @see composeFrameReferences
template<typename InputFrameReferences>
constexpr FrameReferences<
    typename InputFrameReferences::ChildFrame,
    typename InputFrameReferences::ParentFrame>
invertFrameReferences(const InputFrameReferences& input)
{
  using Result = FrameReferences<
      typename InputFrameReferences::ChildFrame,
      typename InputFrameReferences::ParentFrame>;

  return std::make_from_tuple<Result>(
      std::tuple_cat(input.childFrameAsTuple(), input.parentFrameAsTuple()));
}


} // namespace nioc::geometry
