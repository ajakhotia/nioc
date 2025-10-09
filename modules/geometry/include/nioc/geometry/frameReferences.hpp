////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "frameConcepts.hpp"
#include <nioc/common/typeTraits.hpp>
#include <type_traits>

namespace nioc::geometry
{
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
  template<
      typename ParentFrame = typename SelfType::ParentFrame,
      typename ChildFrame = typename SelfType::ChildFrame,
      typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
      typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
  FrameReferences() noexcept: ParentConcept(), ChildConcept()
  {
  }

  /// @brief Constructs with static parent and runtime child frame.
  /// @param childId Child frame identifier (string or DynamicFrame).
  template<
      typename ChildConceptArgs,
      typename ParentFrame = typename SelfType::ParentFrame,
      typename ChildFrame = typename SelfType::ChildFrame,
      typename = typename std::enable_if_t<common::isSpecialization<ParentFrame, StaticFrame>>,
      typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
  [[maybe_unused]] explicit FrameReferences(ChildConceptArgs&& childId) noexcept:
      ParentConcept(), ChildConcept(std::forward<ChildConceptArgs>(childId))
  {
  }

  /// @brief Constructs with runtime parent and static child frame.
  /// @param parentId Parent frame identifier (string or DynamicFrame).
  template<
      typename ParentConceptArgs,
      typename ParentFrame = typename SelfType::ParentFrame,
      typename ChildFrame = typename SelfType::ChildFrame,
      typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
      typename = typename std::enable_if_t<common::isSpecialization<ChildFrame, StaticFrame>>>
  [[maybe_unused]] explicit FrameReferences(ParentConceptArgs&& parentId, int = 0) noexcept:
      ParentConcept(std::forward<ParentConceptArgs>(parentId)), ChildConcept()
  {
  }

  /// @brief Constructs with runtime frame identities.
  /// @param parentId Parent frame identifier (string or DynamicFrame).
  /// @param childId Child frame identifier (string or DynamicFrame).
  template<
      typename ParentConceptArgs,
      typename ChildConceptArgs,
      typename ParentFrame = typename SelfType::ParentFrame,
      typename ChildFrame = typename SelfType::ChildFrame,
      typename = typename std::enable_if_t<std::is_same_v<ParentFrame, DynamicFrame>>,
      typename = typename std::enable_if_t<std::is_same_v<ChildFrame, DynamicFrame>>>
  [[maybe_unused]] FrameReferences(
      ParentConceptArgs&& parentId,
      ChildConceptArgs&& childId) noexcept:
      ParentConcept(std::forward<ParentConceptArgs>(parentId)),
      ChildConcept(std::forward<ChildConceptArgs>(childId))
  {
  }

  virtual ~FrameReferences() = default;
};

/// @brief Exception for mismatched frame composition.
///
/// Thrown when composing transformations with incompatible frames.
class FrameCompositionException: public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

namespace helpers
{
std::string
frameCompositionErrorMessage(const std::string& lhsFrameName, const std::string& rhsFrameName);

/// @brief Asserts frame equality at compile-time.
template<
    typename LhsFrame,
    typename RhsFrame,
    typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and
        common::isSpecialization<RhsFrame, StaticFrame> and LhsFrame::name() == RhsFrame::name()>>
inline constexpr void assertFrameEqual() noexcept
{
}

/// @brief Asserts static frame matches dynamic frame at runtime.
/// @param rhsFrame Dynamic frame to check.
template<
    typename LhsFrame,
    typename RhsFrame,
    typename = typename std::enable_if_t<
        common::isSpecialization<LhsFrame, StaticFrame> and std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const RhsFrame& rhsFrame)
{
  if(LhsFrame::name() != rhsFrame.name())
  {
    auto msg = frameCompositionErrorMessage(std::string(LhsFrame::name()), rhsFrame.name());
    throw FrameCompositionException(std::move(msg));
  }
}

/// @brief Asserts dynamic frame matches static frame at runtime.
/// @param lhsFrame Dynamic frame to check.
template<
    typename LhsFrame,
    typename RhsFrame,
    typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and common::isSpecialization<RhsFrame, StaticFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame)
{
  if(lhsFrame.name() != RhsFrame::name())
  {
    auto msg = frameCompositionErrorMessage(lhsFrame.name(), std::string(RhsFrame::name()));
    throw FrameCompositionException(std::move(msg));
  }
}

/// @brief Asserts two dynamic frames match at runtime.
/// @param lhsFrame First dynamic frame.
/// @param rhsFrame Second dynamic frame.
template<
    typename LhsFrame,
    typename RhsFrame,
    typename = typename std::enable_if_t<
        std::is_same_v<LhsFrame, DynamicFrame> and std::is_same_v<RhsFrame, DynamicFrame>>>
inline void assertFrameEqual(const LhsFrame& lhsFrame, const RhsFrame& rhsFrame)
{
  if(lhsFrame.name() != rhsFrame.name())
  {
    auto msg = frameCompositionErrorMessage(lhsFrame.name(), rhsFrame.name());
    throw FrameCompositionException(std::move(msg));
  }
}


} // namespace helpers

/// @brief Composes two frame relationships.
///
/// Combines two transformations. Throws FrameCompositionException if frames don't match.
///
/// @param lhsFrameReferences First frame relationship.
/// @param rhsFrameReferences Second frame relationship.
/// @return Composed frame relationship.
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
FrameReferences<
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
