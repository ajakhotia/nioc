////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <gtest/gtest.h>
#include <nioc/terminus/consignment.hpp>
#include <nioc/terminus/idl/testSchema.capnp.h>
#include <nioc/terminus/msg.hpp>
#include <optional>
#include <utility>

namespace nioc::terminus
{

TEST(Consignment, countsItselfInFlightForItsLifetime)
{
  auto counter = std::atomic_uint32_t{0};
  const auto msgPtr = std::make_shared<const Msg<TestSchema>>();

  {
    const auto consignment = Consignment{msgPtr, counter};
    EXPECT_EQ(1U, counter.load());

    const auto another = Consignment{msgPtr, counter};
    EXPECT_EQ(2U, counter.load());
  }

  // Both consignments died; the counter is balanced back to zero.
  EXPECT_EQ(0U, counter.load());
}

TEST(Consignment, carriesItsMessageAcrossMoves)
{
  auto counter = std::atomic_uint32_t{0};
  const auto msgPtr = std::make_shared<const Msg<TestSchema>>();

  auto consignment = std::optional<Consignment>{std::in_place, msgPtr, counter};
  EXPECT_EQ(1U, counter.load());

  // Moving transfers the message and the in-flight token: the count stays at one, and only the
  // destination's death releases it.
  auto moved = std::move(*consignment);
  consignment.reset();
  EXPECT_EQ(1U, counter.load());
  EXPECT_EQ(msgPtr.get(), moved.msg().get());
}

} // namespace nioc::terminus
