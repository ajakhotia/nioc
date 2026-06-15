////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <gtest/gtest.h>
#include <nioc/common/makeBimap.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace nioc::common
{
namespace
{

using namespace std::string_literals;

enum class Color : std::uint8_t
{
  Red,
  Green,
  Blue
};

TEST(MakeBimap, looksUpBothDirections)
{
  const auto bimap = makeBimap({
      std::make_pair(Color::Red, "Red"s),
      std::make_pair(Color::Green, "Green"s),
      std::make_pair(Color::Blue, "Blue"s),
  });

  EXPECT_EQ(3U, bimap.size());
  EXPECT_EQ("Green", bimap.left.at(Color::Green));
  EXPECT_EQ(Color::Blue, bimap.right.at("Blue"));
}

TEST(MakeBimap, missingKeyThrows)
{
  const auto bimap = makeBimap({std::make_pair(Color::Red, "Red"s)});

  EXPECT_THROW(static_cast<void>(bimap.left.at(Color::Green)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(bimap.right.at("Green")), std::out_of_range);
}

TEST(MakeBimap, duplicateKeyKeepsFirstPair)
{
  // Map semantics: a pair whose key (on either side) is already present is ignored.
  const auto bimap = makeBimap({
      std::make_pair(Color::Red, "Red"s),
      std::make_pair(Color::Red, "Crimson"s),
      std::make_pair(Color::Green, "Red"s),
  });

  EXPECT_EQ(1U, bimap.size());
  EXPECT_EQ("Red", bimap.left.at(Color::Red));
  EXPECT_EQ(Color::Red, bimap.right.at("Red"));
}

TEST(MakeBimap, buildsFromRuntimeRange)
{
  const auto pairs = std::vector{
      std::make_pair(Color::Red, "Red"s),
      std::make_pair(Color::Green, "Green"s)};

  const auto bimap = makeBimap(pairs);

  EXPECT_EQ(2U, bimap.size());
  EXPECT_EQ("Green", bimap.left.at(Color::Green));
  EXPECT_EQ(Color::Red, bimap.right.at("Red"));
}

TEST(MakeBimap, acceptsTupleLikeElements)
{
  const auto pairs = std::vector{
      std::tuple{ Color::Red,  "Red"s},
      std::tuple{Color::Blue, "Blue"s}
  };

  const auto bimap = makeBimap(pairs);

  EXPECT_EQ(2U, bimap.size());
  EXPECT_EQ("Blue", bimap.left.at(Color::Blue));
  EXPECT_EQ(Color::Red, bimap.right.at("Red"));
}

TEST(MakeBimap, zipsTwoRanges)
{
  const auto colors = std::vector{Color::Red, Color::Green};
  const auto names = std::vector{"Red"s, "Green"s};

  const auto bimap = makeBimap(colors, names);

  EXPECT_EQ(2U, bimap.size());
  EXPECT_EQ("Green", bimap.left.at(Color::Green));
  EXPECT_EQ(Color::Red, bimap.right.at("Red"));
}

} // namespace
} // namespace nioc::common
