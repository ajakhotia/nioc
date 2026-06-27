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
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/reservation.hpp>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/tape.hpp>
#include <nioc/logger/logger.hpp>
#include <span>
#include <utility>

namespace nioc::chronicle
{

Reservation::Reservation(
    Channel& channel,
    std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> rollPtr,
    const std::uint64_t rollId,
    const std::span<std::byte> span):
  mChannelPtr{&channel},
  mRollPtr{std::move(rollPtr)},
  mRollId{rollId},
  mSpan{span}
{
}

Reservation::~Reservation()
{
  if(mRollPtr)
  {
    mChannelPtr->rewind(*this, 0);
  }
}

Reservation& Reservation::operator=(Reservation&& other) noexcept
{
  std::swap(mChannelPtr, other.mChannelPtr);
  std::swap(mRollPtr, other.mRollPtr);
  std::swap(mRollId, other.mRollId);
  std::swap(mSpan, other.mSpan);

  return *this;
}

std::span<std::byte> Reservation::span() const noexcept
{
  return mSpan;
}

void Reservation::modify(const std::size_t newSize)
{
  mChannelPtr->modify(*this, newSize);
}

Crate Reservation::commit(const std::size_t usedSize) &&
{
  const auto offset = static_cast<std::uint64_t>(std::distance(mRollPtr->data(), mSpan.data()));

  if(not mRollPtr->rewind(mSpan, roundUpToWord(usedSize)))
  {
    // A later claim moved the cursor past this reservation, so its unused reserved tail can't be
    // reclaimed and stays stranded - the frame itself still records correctly. Expected once the
    // channel is multi-producer.
    logger::warn("Could not reclaim the reservation's unused tail; a later claim followed it.");
  }

  mChannelPtr->append(
      TimelineEntry{
          .mChannelId = mChannelPtr->id(),
          .mRollId = mRollId,
          .mOffset = offset,
          .mSize = usedSize});

  return Crate{
      std::shared_ptr<const void>(std::move(mRollPtr)),
      std::as_bytes(mSpan.first(usedSize))};
}

} // namespace nioc::chronicle
