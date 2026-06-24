////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/chronicle/crate.hpp>
#include <span>

namespace nioc::chronicle
{
namespace
{

TEST(Crate, copiesShareTheSameBytes)
{
  const auto backing = std::make_shared<std::array<std::byte, 4>>(
      std::array<std::byte, 4>{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}});

  const auto crate = Crate{backing, std::as_bytes(std::span{*backing})};
  const auto copy = crate; // NOLINT(performance-unnecessary-copy-initialization)

  EXPECT_EQ(copy.span().data(), crate.span().data()); // same bytes, not a duplicate
  EXPECT_EQ(copy.span().size(), 4U);
}

TEST(Crate, keepsItsBytesAliveAfterTheProducerIsGone)
{
  auto crate = Crate{};
  {
    auto backing = std::make_shared<std::array<std::byte, 2>>(
        std::array<std::byte, 2>{std::byte{0xAB}, std::byte{0xCD}});
    crate = Crate{backing, std::as_bytes(std::span{*backing})};
  } // the producer's shared_ptr is gone; the crate is the only owner now

  ASSERT_EQ(crate.span().size(), 2U);
  EXPECT_EQ(crate.span()[0], std::byte{0xAB});
  EXPECT_EQ(crate.span()[1], std::byte{0xCD});
}

TEST(Crate, isEmptyByDefault)
{
  const auto crate = Crate{};
  EXPECT_TRUE(crate.span().empty());
}

} // namespace
} // namespace nioc::chronicle
