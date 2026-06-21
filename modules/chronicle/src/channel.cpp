////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "timeline.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <nioc/chronicle/channel.hpp>
#include <span>
#include <utility>

namespace nioc::chronicle
{

Channel::Channel(
    const ChannelId channelId,
    std::filesystem::path channelDir,
    const std::size_t rollCapacity,
    Timeline& timeline):
  mChannelId{channelId},
  mChannelDir{std::move(channelDir)},
  mRollCapacity{rollCapacity},
  mTimeline{timeline}
{
}

Channel::~Channel()
{
  // Trim the active roll to its written bytes, not its full capacity.
  if(mActiveRoll)
  {
    mActiveRoll->shrink_to_fit();
  }
}

ChannelId Channel::id() const noexcept
{
  return mChannelId;
}

Reservation Channel::reserve(const std::size_t size)
{
  const auto reservedSize = roundUpToWord(size);

  auto slot = mActiveRoll ? mActiveRoll->claim(reservedSize) : std::span<std::byte>{};
  if(slot.empty())
  {
    openNewRoll(reservedSize);
    slot = mActiveRoll->claim(reservedSize);
  }

  return Reservation{*this, mActiveRoll, mActiveRollId, slot};
}

Crate Channel::write(const std::span<const std::byte> data)
{
  auto reservation = reserve(data.size());
  std::memcpy(reservation.span().data(), data.data(), data.size());
  return Crate{std::move(reservation), data.size()};
}

void Channel::openNewRoll(const std::size_t minCapacity)
{
  if(mActiveRoll)
  {
    mActiveRoll->shrink_to_fit();
    ++mActiveRollId;
  }

  mActiveRoll = std::make_shared<containers::Tape<containers::MmapArray<std::byte>>>(
      mChannelDir / buildRollName(mActiveRollId),
      std::max(mRollCapacity, minCapacity));
}

void Channel::append(const TimelineEntry& entry)
{
  mTimeline.append(entry);
}

} // namespace nioc::chronicle
