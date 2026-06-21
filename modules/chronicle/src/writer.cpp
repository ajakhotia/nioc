////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "timeline.hpp"
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
    const std::size_t rollCapacity,           // NOLINT(bugprone-easily-swappable-parameters)
    const std::size_t timelineFileCapacity):  // NOLINT(bugprone-easily-swappable-parameters)
  mLogRoot{common::requireEmptyDirectory(std::move(rootDir))},
  mRollCapacity{rollCapacity},
  mTimeline{std::make_unique<Timeline>(mLogRoot / kTimelineDirName, timelineFileCapacity)}
{
  logger::info("Writing chronicle to {} with roll capacity {}.", mLogRoot.string(), mRollCapacity);
}

Writer::~Writer() = default;

Channel& Writer::channel(const ChannelId channelId)
{
  return mLockedChannelMap.execute(
      [this, channelId](ChannelMap& channelMap) -> Channel&
      {
        auto& slot = channelMap[channelId];
        if(not slot)
        {
          slot = std::make_unique<Channel>(
              channelId,
              mLogRoot / hexString(channelId.mValue),
              mRollCapacity,
              *mTimeline);
        }
        return *slot;
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
