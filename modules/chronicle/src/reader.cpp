////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <nioc/chronicle/reader.hpp>
#include <nioc/common/filesystem.hpp>
#include <span>
#include <system_error>
#include <utility>

namespace nioc::chronicle
{

Reader::Reader(std::filesystem::path logRoot):
  mLogRoot{common::requireExistingDirectory(std::move(logRoot))}
{
  // Map the single timeline file if the chronicle recorded anything. A missing or empty (trimmed,
  // never-written) file is a chronicle that recorded nothing - replay nothing.
  const auto timelinePath = mLogRoot / kTimelineFileName;
  auto errorCode = std::error_code{};
  if(const auto byteCount = std::filesystem::file_size(timelinePath, errorCode);
     not errorCode and byteCount > 0)
  {
    mTimelineFile = std::make_unique<const TimelineFile>(timelinePath);
  }
}

Reader::~Reader() = default;

std::optional<Entry> Reader::readNextEntry()
{
  if(not mTimelineFile or mEntryInTimeline >= mTimelineFile->size())
  {
    return std::nullopt;
  }

  const auto& timelineEntry = (*mTimelineFile)[mEntryInTimeline];
  ++mEntryInTimeline;

  auto roll = acquireRoll(timelineEntry.mChannelId, timelineEntry.mRollId);
  const auto span = std::span{*roll}.subspan(timelineEntry.mOffset, timelineEntry.mSize);

  return Entry{
      .mChannelId = timelineEntry.mChannelId,
      .mCrate = Crate{std::move(roll), span}
  };
}

std::shared_ptr<const Reader::Roll> Reader::acquireRoll(
    const ChannelId channelId,
    const std::uint64_t rollId)
{
  auto& cached = mRollCache[channelId][rollId];

  if(auto roll = cached.lock())
  {
    return roll;
  }

  auto roll = std::make_shared<const Roll>(
      mLogRoot / hexString(channelId.mValue) / buildRollName(rollId));
  cached = roll;
  return roll;
}

} // namespace nioc::chronicle
