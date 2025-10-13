////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "streamChannelWriter.hpp"
#include "utils.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nioc/chronicle/writer.hpp>
#include <spdlog/spdlog.h>

namespace nioc::chronicle
{
namespace
{
namespace fs = std::filesystem;

fs::path checkAndSetupLogDirectory(fs::path logRoot)
{
  logRoot /= (iso8601UtcFormat(std::chrono::system_clock::now())) + "_" +
             boost::uuids::to_string(boost::uuids::random_generator_pure()());

  if(fs::exists(logRoot))
  {
    spdlog::warn(
        "[Chronicle::Writer] Directory or file {} exists already. Contents will be cleared.",
        logRoot.string());
    fs::remove_all(logRoot);
  }

  if(not fs::create_directories(logRoot))
  {
    throw std::runtime_error(
        "[Chronicle::Writer] Unable to create root directory for chronicle at " + logRoot.string());
  }

  return logRoot;
}

} // namespace

Writer::Writer(
    std::filesystem::path logRoot,
    const IoMechanism ioMechanism,
    const std::size_t maxFileSizeInBytes):
    mIoMechanism(ioMechanism),
    mLogDirectory(checkAndSetupLogDirectory(std::move(logRoot))),
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
          mLogDirectory / toHexString(channelId),
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
