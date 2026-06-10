////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "streamChannelWriter.hpp"
#include "utils.hpp"
#include <nioc/chronicle/writer.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/filesystem.hpp>
#include <nioc/logger/logger.hpp>

namespace nioc::chronicle
{

Writer::Writer(
    std::filesystem::path rootDir,
    const IoMechanism ioMechanism,
    const std::size_t maxFileSizeInBytes):
  mIoMechanism(ioMechanism),
  mLogDirectory(common::requireEmptyDirectory(std::move(rootDir))),
  mMaxFileSizeInBytes(maxFileSizeInBytes),
  mLockedSequenceFile(mLogDirectory / kSequenceFileName)
{
  logger::info(
      "Writing chronicle to {} with unit file size {}.",
      mLogDirectory.string(),
      mMaxFileSizeInBytes);
}

Writer::~Writer() = default;

void Writer::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
  // TODO(ajakhotia): This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile.execute(
      [&](std::ofstream& sequenceFile)
      { ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{channelId}); });

  mLockedChannelPtrMap.execute(
      [&](ChannelPtrMap& channelPtrMap)
      {
        auto& channel = acquireChannel(channelId, channelPtrMap);
        channel.writeFrame(data);
      });
}

void Writer::write(const ChannelId channelId, std::span<const std::span<const std::byte>> data)
{
  // TODO(ajakhotia): This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile.execute(
      [&](std::ofstream& sequenceFile)
      { ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{channelId}); });

  mLockedChannelPtrMap.execute(
      [&](ChannelPtrMap& channelPtrMap)
      {
        auto& channel = acquireChannel(channelId, channelPtrMap);
        channel.writeFrame(data);
      });
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mLogDirectory;
}

ChannelWriter& Writer::acquireChannel(const ChannelId channelId, ChannelPtrMap& channelPtrMap)
{
  if(not channelPtrMap.contains(channelId))
  {
    std::unique_ptr<ChannelWriter> channelWriter;

    switch(mIoMechanism)
    {
      case IoMechanism::Stream:
        channelWriter = std::make_unique<StreamChannelWriter>(
            mLogDirectory / hexString(channelId.mValue),
            mMaxFileSizeInBytes);
        break;

      case IoMechanism::Mmap:
        common::throwException<std::invalid_argument>(
            "IoMechanism '{}' is not supported for writing. Use '{}' instead.",
            stringFromIoMechanism(IoMechanism::Mmap),
            stringFromIoMechanism(IoMechanism::Stream));

      default:
        common::throwException<std::invalid_argument>(
            "Unknown IoMechanism with value: {}",
            static_cast<int>(mIoMechanism));
    }

    channelPtrMap.try_emplace(channelId, std::move(channelWriter));
  }

  return *channelPtrMap.at(channelId);
}

} // namespace nioc::chronicle
