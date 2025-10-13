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

/// @brief Creates a bidirectional map from an initializer list of pairs.
///
/// Utility function that creates boost::bimap instances with automatic type deduction.
/// Useful for enum-to-string conversions and other bidirectional mappings.
///
/// @tparam PairType Type of pair (deduced from initializer list).
/// @param list Initializer list of pairs to insert into the bimap.
/// @return Populated boost::bimap instance.
///
/// Example:
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
      [&](const auto& item)
      {
        result.insert({ item.first, item.second });
      });
  return result;
}

} // namespace nioc::common
