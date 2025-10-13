////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapChannelReader.hpp"
#include "utils.hpp"
#include <nioc/chronicle/reader.hpp>

namespace nioc::chronicle
{

Reader::Reader(std::filesystem::path logRoot, const IoMechanism ioMechanism):
    mIoMechanism(ioMechanism),
    mLogRoot(validatePath(std::move(logRoot))),
    mSequenceFile(mLogRoot / kSequenceFileName)
{
}

Reader::~Reader() = default;

Entry Reader::read()
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

  return mLockedChannelReaderMap(
      [&](ChannelReaderMap& channelReaderMap) -> Entry
      {
        auto& channelReader = acquireChannel(sequenceEntry.mChannelId, channelReaderMap);
        return { .mChannelId = sequenceEntry.mChannelId, .mMemoryCrate = channelReader.read() };
      });
}

ChannelReader& Reader::acquireChannel(ChannelId channelId, ChannelReaderMap& channelReaderMap)
{
  if(not channelReaderMap.contains(channelId))
  {
    std::unique_ptr<ChannelReader> channelReader;

    switch(mIoMechanism)
    {
    case IoMechanism::Mmap:
      channelReader = std::make_unique<MmapChannelReader>(mLogRoot / toHexString(channelId));
      break;

    case IoMechanism::Stream:
      throw std::invalid_argument(
          "[Chronicle::Reader] IoMechanism '" + stringFromIoMechanism(IoMechanism::Stream) +
          "' is not supported for reading. Use '" + stringFromIoMechanism(IoMechanism::Mmap) + "' instead.");

    default:
      throw std::invalid_argument(
          "[Chronicle::Reader] Unknown IoMechanism with value: " +
          std::to_string(static_cast<int>(mIoMechanism)));
    }

    channelReaderMap.try_emplace(channelId, std::move(channelReader));
  }

  return *channelReaderMap.at(channelId);
}

} // namespace nioc::chronicle
