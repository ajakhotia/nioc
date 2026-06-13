////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/bimap.hpp>
#include <initializer_list>
#include <utility>

namespace nioc::common
{

/// @brief Builds a two-way map from a list of pairs.
///
/// Key and value types come from the pairs. Useful for enum-to-string tables. If a key already
/// exists on either side, the new pair is dropped: the first pair wins.
///
/// @tparam PairType Pair type, deduced from the list.
/// @param list Pairs to insert.
/// @return The filled boost::bimap.
///
/// @code
/// using namespace std::string_literals;
/// enum class Color { Red, Green, Blue };
/// const auto& kColorBimap = makeBimap({
///   std::make_pair(Color::Red, "Red"s),
///   std::make_pair(Color::Green, "Green"s),
///   std::make_pair(Color::Blue, "Blue"s)
/// });
/// @endcode
template<typename PairType>
auto makeBimap(std::initializer_list<PairType> list)
{
  auto result = boost::bimap<typename PairType::first_type, typename PairType::second_type>();
  std::for_each(
      list.begin(),
      list.end(),
      [&](const auto& item) { result.insert({item.first, item.second}); });
  return result;
}

} // namespace nioc::common
