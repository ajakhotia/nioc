////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "streamChannelWriter.hpp"
#include "utils.hpp"
#include <nioc/chronicle/writer.hpp>
#include <spdlog/spdlog.h>

namespace nioc::chronicle
{
namespace
{
namespace fs = std::filesystem;

fs::path requireEmptyDirectory(fs::path path)
{
  if(not fs::exists(path))
  {
    throw std::invalid_argument(
        "[Chronicle::Writer] Directory does not exist: " + path.string());
  }

  if(not fs::is_directory(path))
  {
    throw std::invalid_argument(
        "[Chronicle::Writer] Path is not a directory: " + path.string());
  }

  if(not fs::is_empty(path))
  {
    throw std::invalid_argument(
        "[Chronicle::Writer] Directory is not empty: " + path.string());
  }

  return path;
}

} // namespace

Writer::Writer(
    std::filesystem::path rootDir,
    const IoMechanism ioMechanism,
    const std::size_t maxFileSizeInBytes):
    mIoMechanism(ioMechanism),
    mLogDirectory(requireEmptyDirectory(std::move(rootDir))),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mLockedSequenceFile(mLogDirectory / kSequenceFileName)
{
  spdlog::info(
      "[Chronicle::Writer] Writing chronicle to {} with unit file size {}.",
      mLogDirectory.string(),
      mMaxFileSizeInBytes);
}

Writer::~Writer() = default;

void Writer::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
  // TODO(ajakhotia): This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile(
      [&](std::ofstream& sequenceFile)
      {
        ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{ channelId });
      });

  mLockedChannelPtrMap(
      [&](ChannelPtrMap& channelPtrMap)
      {
        auto& channel = acquireChannel(channelId, channelPtrMap);
        channel.writeFrame(data);
      });
}

void Writer::write(const ChannelId channelId, std::span<const std::span<const std::byte>> data)
{
  // TODO(ajakhotia): This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile(
      [&](std::ofstream& sequenceFile)
      {
        ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{ channelId });
      });

  mLockedChannelPtrMap(
      [&](ChannelPtrMap& channelPtrMap)
      {
        auto& channel = acquireChannel(channelId, channelPtrMap);
        channel.writeFrame(data);
      });
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
            mLogDirectory / toHexString(channelId.mValue),
            mMaxFileSizeInBytes);
        break;

      case IoMechanism::Mmap:
        throw std::invalid_argument(
            "[Chronicle::Writer] IoMechanism '" + stringFromIoMechanism(IoMechanism::Mmap) +
            "' is not supported for writing. Use '" + stringFromIoMechanism(IoMechanism::Stream) +
            "' instead.");

      default:
        throw std::invalid_argument(
            "[Chronicle::Writer] Unknown IoMechanism with value: " +
            std::to_string(static_cast<int>(mIoMechanism)));
    }

    channelPtrMap.try_emplace(channelId, std::move(channelWriter));
  }

  return *channelPtrMap.at(channelId);
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mLogDirectory;
}

} // namespace nioc::chronicle
