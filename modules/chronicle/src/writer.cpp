////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include <memory>
#include <nioc/chronicle/channel.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/filesystem.hpp>
#include <nioc/logger/logger.hpp>
#include <utility>

namespace nioc::chronicle
{

Writer::Writer(
    std::filesystem::path rootDir,
    const std::size_t rollCapacity,      // NOLINT(bugprone-easily-swappable-parameters)
    const std::size_t timelineCapacity): // NOLINT(bugprone-easily-swappable-parameters)
  mLogRoot{common::requireEmptyDirectory(std::move(rootDir))},
  mRollCapacity{rollCapacity},
  mTimeline{mLogRoot / kTimelineFileName, timelineCapacity / sizeof(TimelineEntry)}
{
  logger::info("Writing chronicle to {} with roll capacity {}.", mLogRoot.string(), mRollCapacity);
}

Writer::~Writer()
{
  mTimeline.shrink_to_fit();
}

Channel& Writer::channel(const ChannelId channelId)
{
  return mLockedChannelMap.execute(
      [this, channelId](ChannelMap& channelMap) -> Channel&
      {
        auto& channelPtr = channelMap[channelId];
        if(not channelPtr)
        {
          channelPtr = std::make_unique<Channel>(
              channelId,
              mLogRoot / hexString(channelId.mValue),
              mRollCapacity,
              mTimeline);
        }
        return *channelPtr;
      });
}

Crate Writer::write(const ChannelId channelId, const std::span<const std::byte> data)
{
  return channel(channelId).write(data);
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mLogRoot;
}

} // namespace nioc::chronicle
