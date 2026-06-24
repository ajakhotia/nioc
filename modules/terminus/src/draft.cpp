////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/any.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/reservation.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <nioc/terminus/draft.hpp>
#include <stdexcept>
#include <utility>

namespace nioc::terminus
{

chronicle::Crate flattenDraft(ArenaMessageBuilder& builder, chronicle::Reservation reservation)
{
  if(builder.overflowed()) [[unlikely]]
  {
    // The message outgrew its arena onto the heap. Re-root it into a single segment so the recorded
    // frame keeps the fast path's single-segment shape. The busted length bounds the collapsed
    // size, so sizing the rebuild's first segment to it lands the re-root in one segment.
    auto singleSegmentBuilder = capnp::MallocMessageBuilder{
        static_cast<unsigned int>(capnp::computeSerializedSizeInWords(builder))};
    singleSegmentBuilder.setRoot(builder.getRoot<capnp::AnyPointer>().asReader());

    // Frame that single segment straight into the reservation - no intermediate serialization. The
    // segment lives in the rebuild's own memory, so it survives resizing the reservation (which
    // reuses the reservation's bytes).
    const auto segments = singleSegmentBuilder.getSegmentsForOutput();
    if(segments.size() != 1)
    {
      common::throwException<std::logic_error>(
          "Re-rooted message has {} segments; expected a single segment.",
          segments.size());
    }

    reservation.modify(ArenaMessageBuilder::frameSize(segments[0].size()));
    const auto frame = ArenaMessageBuilder::writeFrame(reservation.span(), segments[0]);
    return std::move(reservation).commit(frame.size());
  }

  // The message stayed within the arena as a single segment, so the frame is already in place:
  // stamp its header and commit — zero copy.
  const auto frame = builder.frame();
  return std::move(reservation).commit(frame.size());
}

} // namespace nioc::terminus
