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

/// A tuple-like type with exactly two elements (e.g. std::pair, or a two-element std::tuple or
/// std::array): the value carried by a @ref makeBimap input.
template<typename T>
concept PairLike = requires { requires std::tuple_size<std::remove_cvref_t<T>>::value == 2; };

/// A range whose elements are pair-like.
template<typename T>
concept PairRange = std::ranges::input_range<T> and PairLike<std::ranges::range_value_t<T>>;

/// The single insertion: each element's first member keys the left side, its second the right; the
/// first pair for a key on either side wins. The bimap's key types are deduced from the pair.
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

/// @brief Builds a two-way map from a range of pair-like elements.
///
/// @tparam Range Range whose elements are pair-like (a std::pair, or a two-element std::tuple or
/// std::array). The bimap's two key types are deduced from the element.
///
/// @param pairs Pairs to insert; the first pair for a key on either side wins.
///
/// @return The filled boost::bimap.
///
/// @code
/// using namespace std::string_literals;
/// const auto bimap = makeBimap(std::vector{std::pair{1, "Red"s}, std::pair{2, "Green"s}});
/// // bimap.left.at(1) == "Red"; bimap.right.at("Green") == 2
/// @endcode
template<detail::PairRange Range>
auto makeBimap(const Range& pairs)
{
  return detail::makeBimap(pairs);
}

/// @brief Builds a two-way map from a braced list of pair-like elements.
///
/// Template deduction cannot see through bare nested braces, so each element must be a typed pair
/// (e.g. `std::pair{a, b}`), not `{a, b}`.
///
/// @tparam Pair Pair-like element type.
///
/// @param pairs Pairs to insert; the first pair for a key on either side wins.
///
/// @return The filled boost::bimap.
template<detail::PairLike Pair>
auto makeBimap(std::initializer_list<Pair> pairs)
{
  return detail::makeBimap(pairs);
}

/// @brief Builds a two-way map by pairing a range of left keys with a range of right keys.
///
/// Pairs elements positionally and stops at the shorter range. The first pair for a key on either
/// side wins.
///
/// @tparam LeftRange Range of left keys.
///
/// @tparam RightRange Range of right keys.
///
/// @param lefts Left keys.
///
/// @param rights Right keys.
///
/// @return The filled boost::bimap.
template<std::ranges::input_range LeftRange, std::ranges::input_range RightRange>
auto makeBimap(const LeftRange& lefts, const RightRange& rights)
{
  return detail::makeBimap(std::views::zip(lefts, rights));
}

} // namespace nioc::common
