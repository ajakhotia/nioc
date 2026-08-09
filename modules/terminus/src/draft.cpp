////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/reservation.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <nioc/terminus/draft.hpp>
#include <nioc/terminus/utils.hpp>
#include <utility>

namespace nioc::terminus
{

chronicle::Crate flattenDraft(ArenaMessageBuilder& builder, chronicle::Reservation reservation)
{
  if(builder.overflowed()) [[unlikely]]
  {
    // The message outgrew its arena onto the heap. Re-root it into a single segment so the recorded
    // frame keeps the fast path's single-segment shape.
    const auto collapsed = collapseToSingleSegment(builder);

    // Frame that single segment straight into the reservation - no intermediate serialization. The
    // segment lives in the rebuild's own memory, so it survives resizing the reservation (which
    // reuses the reservation's bytes).
    const auto segment = collapsed->getSegmentsForOutput().front();
    reservation.modify(ArenaMessageBuilder::frameSize(segment.size()));
    const auto frame = ArenaMessageBuilder::writeFrame(reservation.span(), segment);
    return std::move(reservation).commit(frame.size());
  }

  // The message stayed within the arena as a single segment, so the frame is already in place:
  // stamp its header and commit — zero copy.
  const auto frame = builder.frame();
  return std::move(reservation).commit(frame.size());
}

} // namespace nioc::terminus
