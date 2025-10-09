////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "logReaderImpl.hpp"
#include "utils.hpp"

namespace nioc::chronicle
{

Reader::LogReaderImpl::LogReaderImpl(std::filesystem::path logRoot):
    mLogRoot(validatePath(std::move(logRoot))), mSequenceFile(mLogRoot / kSequenceFileName)
{
}

Entry Reader::LogReaderImpl::read()
{
  const auto indexPtrOffset = mNextReadIndex * sizeof(SequenceEntry);

  if(indexPtrOffset >= mSequenceFile.size())
  {
    throw std::runtime_error(
        "Reached end of sequence file at " + (mLogRoot / kSequenceFileName).string());
  }

  ++mNextReadIndex;

  const auto sequenceEntry = ReadWriteUtil<SequenceEntry>::read(
      std::next(mSequenceFile.data(), static_cast<ssize_t>(indexPtrOffset)));

  auto& channelReader = acquireChannel(sequenceEntry.mChannelId);
  return { .mChannelId = sequenceEntry.mChannelId, .mMemoryCrate = channelReader.read() };
}

ChannelReader& Reader::LogReaderImpl::acquireChannel(ChannelId channelId)
{
  return mLockedChannelReaderMap(
      [&](ChannelReaderMap& channelReaderMap) -> ChannelReader&
      {
        if(not channelReaderMap.contains(channelId))
        {
          channelReaderMap.try_emplace(channelId, ChannelReader(mLogRoot / toHexString(channelId)));
        }

        return channelReaderMap.at(channelId);
      });
}


} // namespace nioc::chronicle
