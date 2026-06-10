////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmapChannelReader.hpp"
#include "utils.hpp"
#include <nioc/chronicle/reader.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/filesystem.hpp>

namespace nioc::chronicle
{

Reader::Reader(std::filesystem::path logRoot, const IoMechanism ioMechanism):
  mIoMechanism(ioMechanism),
  mLogRoot(common::requireExistingDirectory(std::move(logRoot))),
  mSequenceFile(mLogRoot / kSequenceFileName)
{
}

Reader::~Reader() = default;

Entry Reader::read()
{
  const auto indexPtrOffset = mNextReadIndex * sizeof(SequenceEntry);

  if(indexPtrOffset >= mSequenceFile.size())
  {
    common::throwException<std::runtime_error>(
        "Reached end of sequence file at {}",
        (mLogRoot / kSequenceFileName).string());
  }

  ++mNextReadIndex;

  const auto sequenceEntry = ReadWriteUtil<SequenceEntry>::read(
      std::next(mSequenceFile.data(), static_cast<ssize_t>(indexPtrOffset)));

  return mLockedChannelReaderMap.execute(
      [&](ChannelReaderMap& channelReaderMap) -> Entry
      {
        auto& channelReader = acquireChannel(sequenceEntry.mChannelId, channelReaderMap);
        return {.mChannelId = sequenceEntry.mChannelId, .mMemoryCrate = channelReader.read()};
      });
}

ChannelReader& Reader::acquireChannel(const ChannelId channelId, ChannelReaderMap& channelReaderMap)
{
  if(not channelReaderMap.contains(channelId))
  {
    std::unique_ptr<ChannelReader> channelReader;

    switch(mIoMechanism)
    {
      case IoMechanism::Mmap:
        channelReader = std::make_unique<MmapChannelReader>(mLogRoot / hexString(channelId.mValue));
        break;

      case IoMechanism::Stream:
        common::throwException<std::invalid_argument>(
            "IoMechanism '{}' is not supported for reading. Use '{}' instead.",
            stringFromIoMechanism(IoMechanism::Stream),
            stringFromIoMechanism(IoMechanism::Mmap));

      default:
        common::throwException<std::invalid_argument>(
            "Unknown IoMechanism with value: {}",
            static_cast<int>(mIoMechanism));
    }

    channelReaderMap.try_emplace(channelId, std::move(channelReader));
  }

  return *channelReaderMap.at(channelId);
}

} // namespace nioc::chronicle
