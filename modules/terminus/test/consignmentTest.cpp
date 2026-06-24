////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <nioc/chronicle/crate.hpp>
#include <nioc/terminus/consignment.hpp>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace
{

/// A self-backed crate over a small heap buffer, for exercising Consignment without a chronicle.
chronicle::Crate makeCrate()
{
  constexpr auto kBufferSize = std::size_t{8};
  auto storage = std::make_shared<std::vector<std::byte>>(kBufferSize);
  const auto span = std::as_bytes(std::span{*storage});
  return chronicle::Crate{std::move(storage), span};
}

} // namespace

TEST(Consignment, countsItselfInFlightForItsLifetime)
{
  auto counter = std::atomic_uint32_t{0};
  const auto crate = makeCrate();

  {
    const auto consignment = Consignment{crate, counter};
    EXPECT_EQ(1U, counter.load());

    const auto another = Consignment{crate, counter};
    EXPECT_EQ(2U, counter.load());
  }

  // Both consignments died; the counter is balanced back to zero.
  EXPECT_EQ(0U, counter.load());
}

TEST(Consignment, carriesItsCrateAcrossMoves)
{
  auto counter = std::atomic_uint32_t{0};
  const auto crate = makeCrate();

  auto consignment = std::optional<Consignment>{std::in_place, crate, counter};
  EXPECT_EQ(1U, counter.load());

  // Moving transfers the crate and the in-flight token: the count stays at one, and only the
  // destination's death releases it.
  auto moved = std::move(*consignment);
  consignment.reset();
  EXPECT_EQ(1U, counter.load());
  EXPECT_EQ(crate.span().data(), moved.crate().span().data());
}

} // namespace nioc::terminus
