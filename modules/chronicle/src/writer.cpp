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

} // End of anonymous namespace.

Writer::Writer(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
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

void Writer::write(const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
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
    channelPtrMap.try_emplace(
        channelId,
        std::make_unique<StreamChannelWriter>(
            mLogDirectory / toHexString(channelId), mMaxFileSizeInBytes));
  }

  return *channelPtrMap.at(channelId);
}

const std::filesystem::path& Writer::path() const noexcept
{
  return mLogDirectory;
}

} // End of namespace nioc::chronicle
