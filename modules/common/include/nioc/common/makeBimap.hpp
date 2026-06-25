////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/bimap.hpp>
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace nioc::common
{
namespace detail
{

/// @brief Satisfied by any type that decomposes into exactly two tuple elements, such as
/// `std::pair`, a size-2 `std::tuple`, or a size-2 `std::array`.
///
/// Element 0 is treated as the left key and element 1 as the right key.
///
/// @tparam T The candidate pair-like type. Cv-qualifiers and references are ignored.
template<typename T>
concept PairLike = requires { requires std::tuple_size<std::remove_cvref_t<T>>::value == 2; };

/// @brief Satisfied by an input range whose element type is `PairLike`.
///
/// @tparam T The candidate range type.
///
/// @see PairLike
template<typename T>
concept PairRange = std::ranges::input_range<T> and PairLike<std::ranges::range_value_t<T>>;

/// @brief Shared implementation that all public `makeBimap` overloads funnel into: builds a
/// `boost::bimap` from a range of pairs, taking element 0 of each pair as the left key and element
/// 1 as the right key.
///
/// The left and right key types are the pair element types with const, volatile, and reference
/// qualifiers removed. Keys are copied into the result. Pairs are inserted in iteration order, and
/// any pair whose left or right key already exists is dropped, so the first occurrence of each key
/// wins.
///
/// @tparam Range A `PairRange`; its element type must decompose into exactly two elements.
///
/// @param pairs The range of pairs to insert, read in iteration order.
///
/// @return A `boost::bimap` keyed by the cvref-stripped pair element types.
///
/// @see makeBimap(const Range&), makeBimap(std::initializer_list<Pair>)
template<PairRange Range>
auto makeBimap(const Range& pairs)
{
  using std::get;
  using PairType = std::ranges::range_value_t<Range>;
  auto result = boost::bimap<
      std::remove_cvref_t<std::tuple_element_t<0, PairType>>,
      std::remove_cvref_t<std::tuple_element_t<1, PairType>>>();
  for(const auto& pair: pairs)
  {
    result.insert({get<0>(pair), get<1>(pair)});
  }
  return result;
}

} // namespace detail

/// @brief Builds a `boost::bimap` from a range of pairs, using element 0 of each pair as the left
/// key and element 1 as the right key.
///
/// Example:
///
///     std::vector<std::pair<int, std::string>> data{{1, "one"}, {2, "two"}};
///     auto bimap = makeBimap(data);  // bimap.left.at(1) == "one"
///
/// The bimap's left and right key types are the pair element types with const, volatile, and
/// reference qualifiers removed. Keys are copied into the result, so the source range need not
/// outlive it. Both sides stay unique: pairs are inserted in iteration order, and any pair whose
/// left or right key already exists is dropped, so the first occurrence of each key wins.
///
/// @return A `boost::bimap` keyed by the cvref-stripped pair element types.
///
/// @see PairRange
template<detail::PairRange Range>
auto makeBimap(const Range& pairs)
{
  return detail::makeBimap(pairs);
}

/// @brief Builds a `boost::bimap` directly from a braced list of pairs.
///
/// Use this overload to write the pairs inline, for example
/// `makeBimap({std::pair{1, "one"}, std::pair{2, "two"}})`. Insertion order and duplicate handling
/// match the range overload: first occurrence of each key wins.
///
/// @return A `boost::bimap` keyed by the cvref-stripped pair element types.
///
/// @see makeBimap(const Range&)
template<detail::PairLike Pair>
auto makeBimap(std::initializer_list<Pair> pairs)
{
  return detail::makeBimap(pairs);
}

/// @brief Builds a `boost::bimap` from two parallel ranges, pairing each element of @p lefts with
/// the element of @p rights at the same position.
///
/// Example:
///
///     std::vector ids{1, 2, 3};
///     std::vector names{"a", "b", "c"};
///     auto bimap = makeBimap(ids, names);  // bimap.left.at(2) == "b"
///
/// The bimap's left and right key types are the @p lefts and @p rights element types with const,
/// volatile, and reference qualifiers removed. Pairing stops at the shorter range, so trailing
/// elements of the longer range are ignored. Duplicate handling matches the range overload: first
/// occurrence of each key wins.
///
/// @return A `boost::bimap` keyed by the cvref-stripped element types of the two ranges.
///
/// @see makeBimap(const Range&)
template<std::ranges::input_range LeftRange, std::ranges::input_range RightRange>
auto makeBimap(const LeftRange& lefts, const RightRange& rights)
{
  return detail::makeBimap(std::views::zip(lefts, rights));
}

} // namespace nioc::common
