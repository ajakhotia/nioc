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
/// @brief Normalizes a frame identifier for safe passing.
///
/// Copies a string literal into an owned @ref std::string. Returns any other type
/// (@ref std::string, @ref DynamicFrame) unchanged.
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

/// @brief Holds the parent and child frame identities of a transformation.
///
/// Each frame may be compile-time (StaticFrame) or runtime (DynamicFrame).
///
/// @tparam ParentFrame_ Parent frame type (StaticFrame or DynamicFrame).
/// @tparam ChildFrame_ Child frame type (StaticFrame or DynamicFrame).
template<
    typename ParentFrame_,
    typename ChildFrame_,
    typename ParentConcept = ParentConceptTmpl<ParentFrame_>,
    typename ChildConcept = ChildConceptTmpl<ChildFrame_>>
class FrameReferences: public ParentConcept, public ChildConcept
{
public:
  using SelfType = FrameReferences<ParentFrame_, ChildFrame_>;

  /// @brief Constructs when both frames are static. Takes no arguments.
  template<typename ParentFrame = SelfType::ParentFrame, typename ChildFrame = SelfType::ChildFrame>
  FrameReferences() noexcept
    requires(common::isSpecialization<ParentFrame, StaticFrame> and
             common::isSpecialization<ChildFrame, StaticFrame>)
    : ParentConcept(), ChildConcept()
  {
  }

  /// @brief Constructs with a static parent and a runtime child.
  /// @param childId Child frame identifier. Pass a string or a DynamicFrame.
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

  /// @brief Constructs with a runtime parent and a static child.
  /// @param parentId Parent frame identifier. Pass a string or a DynamicFrame.
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

  /// @brief Constructs when both frames are runtime.
  /// @param parentId Parent frame identifier. Pass a string or a DynamicFrame.
  /// @param childId Child frame identifier. Pass a string or a DynamicFrame.
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

/// @brief Thrown when composing transformations whose inner frames do not match.
class FrameCompositionException final: public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

namespace helpers
{
/// @brief Builds the error message for a @ref FrameCompositionException.
///
/// @param lhsFrameName Name of the left frame in the composition.
/// @param rhsFrameName Name of the right frame in the composition.
/// @return The formatted error message.
std::string frameCompositionErrorMessage(
    const std::string& lhsFrameName,
    const std::string& rhsFrameName);

/// @brief Checks two static frames match. Fails to compile if they differ.
template<typename LhsFrame, typename RhsFrame>
constexpr void assertFrameEqual() noexcept
  requires(
      common::isSpecialization<LhsFrame, StaticFrame> and
      common::isSpecialization<RhsFrame, StaticFrame> and LhsFrame::name() == RhsFrame::name())
{
}

/// @brief Checks a static frame matches a dynamic frame.
/// @param rhsFrame Dynamic frame to check against the static LhsFrame.
/// @throws FrameCompositionException If the frame names differ.
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

/// @brief Checks a dynamic frame matches a static frame.
/// @param lhsFrame Dynamic frame to check against the static RhsFrame.
/// @throws FrameCompositionException If the frame names differ.
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

/// @brief Checks two dynamic frames match.
/// @param lhsFrame First dynamic frame.
/// @param rhsFrame Second dynamic frame.
/// @throws FrameCompositionException If the frame names differ.
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

/// @brief Chains two frame relationships end to end into one.
///
/// The child of the left must match the parent of the right. The result runs from the left
/// parent to the right child.
///
/// @param lhsFrameReferences Left frame relationship.
/// @param rhsFrameReferences Right frame relationship.
/// @return The chained frame relationship.
/// @throws FrameCompositionException If the inner frames do not match.
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

/// @brief Swaps the parent and child of a frame relationship.
/// @param input Frame relationship to invert.
/// @return The inverted frame relationship.
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
