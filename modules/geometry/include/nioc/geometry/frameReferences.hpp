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
/// @brief Normalizes a frame identifier so it can be passed on without array-to-pointer decay.
///
/// A character buffer (such as a string literal) becomes an owned @ref std::string through an
/// explicit cast; an identifier of any other type (@ref std::string, @ref DynamicFrame) is
/// forwarded unchanged.
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

/// @brief Manages parent and child frame relationships.
///
/// Stores frame identities for geometric transformations. Supports both compile-time (StaticFrame)
/// and runtime (DynamicFrame) frame types.
///
/// @tparam ParentFrame_ Parent frame type.
/// @tparam ChildFrame_ Child frame type.
template<
    typename ParentFrame_,
    typename ChildFrame_,
    typename ParentConcept = ParentConceptTmpl<ParentFrame_>,
    typename ChildConcept = ChildConceptTmpl<ChildFrame_>>
class FrameReferences: public ParentConcept, public ChildConcept
{
public:
  using SelfType = FrameReferences<ParentFrame_, ChildFrame_>;

  /// @brief Constructs with compile-time frame identities.
  template<typename ParentFrame = SelfType::ParentFrame, typename ChildFrame = SelfType::ChildFrame>
  FrameReferences() noexcept
    requires(common::isSpecialization<ParentFrame, StaticFrame> and
             common::isSpecialization<ChildFrame, StaticFrame>)
    : ParentConcept(), ChildConcept()
  {
  }

  /// @brief Constructs with static parent and runtime child frame.
  /// @param childId Child frame identifier (string or DynamicFrame).
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

  /// @brief Constructs with runtime parent and static child frame.
  /// @param parentId Parent frame identifier (string or DynamicFrame).
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

  /// @brief Constructs with runtime frame identities.
  /// @param parentId Parent frame identifier (string or DynamicFrame).
  /// @param childId Child frame identifier (string or DynamicFrame).
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

/// @brief Exception for mismatched frame composition.
///
/// Thrown when composing transformations with incompatible frames.
class FrameCompositionException final: public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

namespace helpers
{
/// @brief Builds a @ref FrameCompositionException message from the two clashing frame names.
///
/// @param lhsFrameName Name of the frame on the left of the composition.
///
/// @param rhsFrameName Name of the frame on the right of the composition.
///
/// @return The formatted error message.
std::string frameCompositionErrorMessage(
    const std::string& lhsFrameName,
    const std::string& rhsFrameName);

/// @brief Asserts frame equality at compile-time.
template<typename LhsFrame, typename RhsFrame>
constexpr void assertFrameEqual() noexcept
  requires(
      common::isSpecialization<LhsFrame, StaticFrame> and
      common::isSpecialization<RhsFrame, StaticFrame> and LhsFrame::name() == RhsFrame::name())
{
}

/// @brief Asserts static frame matches dynamic frame at runtime.
/// @param rhsFrame Dynamic frame to check.
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

/// @brief Asserts dynamic frame matches static frame at runtime.
/// @param lhsFrame Dynamic frame to check.
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

/// @brief Asserts two dynamic frames match at runtime.
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

/// @brief Composes two frame relationships into one.
///
/// Combines two transformations end to end.
///
/// @param lhsFrameReferences First frame relationship.
/// @param rhsFrameReferences Second frame relationship.
/// @return Composed frame relationship.
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

/// @brief Inverts a frame relationship.
/// @param input Frame relationship to invert.
/// @return Inverted frame relationship.
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
