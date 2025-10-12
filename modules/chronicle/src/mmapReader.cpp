////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapReader.hpp"
#include "utils.hpp"

namespace nioc::chronicle
{

Reader::MmapReader::MmapReader(std::filesystem::path logRoot):
    mLogRoot(validatePath(std::move(logRoot))), mSequenceFile(mLogRoot / kSequenceFileName)
{
}

Entry Reader::MmapReader::read()
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

MmapChannelReader& Reader::MmapReader::acquireChannel(ChannelId channelId)
{
  return mLockedChannelReaderMap(
      [&](ChannelReaderMap& channelReaderMap) -> MmapChannelReader&
      {
        if(not channelReaderMap.contains(channelId))
        {
          channelReaderMap.try_emplace(channelId, MmapChannelReader(mLogRoot / toHexString(channelId)));
        }

        return channelReaderMap.at(channelId);
      });
}


} // namespace nioc::chronicle
