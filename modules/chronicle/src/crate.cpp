////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <nioc/chronicle/channel.hpp>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/reservation.hpp>
#include <span>
#include <utility>

namespace nioc::chronicle
{

Crate::Crate(Reservation reservation, const std::size_t usedSize)
{
  const auto offset = static_cast<std::uint64_t>(
      std::distance(reservation.mRollPtr->data(), reservation.mSpan.data()));

  // Give back the part of the reservation the frame did not use, so the next frame abuts it, then
  // record the frame's place in the channel's timeline.
  reservation.mRollPtr->rewind(reservation.mSpan, roundUpToWord(usedSize));
  reservation.mChannel->append(TimelineEntry{
      .mChannelId = reservation.mChannel->id(),
      .mRollId = reservation.mRollId,
      .mOffset = offset,
      .mSize = usedSize});

  mBacking = std::shared_ptr<const void>(std::move(reservation.mRollPtr));
  mSpan = std::as_bytes(reservation.mSpan.first(usedSize));
}

} // namespace nioc::chronicle
