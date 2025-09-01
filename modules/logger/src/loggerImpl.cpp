////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "loggerImpl.hpp"
#include "utils.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <spdlog/spdlog.h>

namespace nioc::logger
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
        "[logger] Directory or file {} exists already. Contents will be cleared.",
        logRoot.string());
    fs::remove_all(logRoot);
  }

  if(not fs::create_directories(logRoot))
  {
    throw std::runtime_error(
        "[Logger::Logger] Unable to create root directory for log at " + logRoot.string());
  }

  return logRoot;
}

} // End of anonymous namespace.

Logger::LoggerImpl::LoggerImpl(std::filesystem::path logRoot, const std::size_t maxFileSizeInBytes):
    mLogDirectory(checkAndSetupLogDirectory(std::move(logRoot))),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mLockedSequenceFile(mLogDirectory / kSequenceFileName)
{
  spdlog::info(
      "[Logger] Logging to {} with unit file size {}.",
      mLogDirectory.string(),
      mMaxFileSizeInBytes);
}

void Logger::LoggerImpl::write(const ChannelId channelId, const std::span<const std::byte>& data)
{
  // TODO: This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile(
      [&](std::ofstream& sequenceFile)
      { ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{ channelId }); });

  auto& lockedChannel = acquireChannel(channelId);
  lockedChannel([&](Channel& channel) { channel.writeFrame(data); });
}

void Logger::LoggerImpl::write(
    const ChannelId channelId, const std::vector<std::span<const std::byte>>& data)
{
  // TODO: This can be improved to use fewer locks and avoid race conditions.
  mLockedSequenceFile(
      [&](std::ofstream& sequenceFile)
      { ReadWriteUtil<SequenceEntry>::write(sequenceFile, SequenceEntry{ channelId }); });

  auto& lockedChannel = acquireChannel(channelId);
  lockedChannel([&](Channel& channel) { channel.writeFrame(data); });
}

Logger::LoggerImpl::LockedChannel& Logger::LoggerImpl::acquireChannel(const ChannelId channelId)
{
  return mLockedChannelPtrMap(
      [&](ChannelPtrMap& channelPtrMap) -> LockedChannel&
      {
        if(not channelPtrMap.contains(channelId))
        {
          channelPtrMap.try_emplace(
              channelId,
              std::make_unique<LockedChannel>(
                  mLogDirectory / toHexString(channelId), mMaxFileSizeInBytes));
        }

        return *channelPtrMap.at(channelId);
      });
}

const std::filesystem::path& Logger::LoggerImpl::path() const noexcept
{
  return mLogDirectory;
}


} // namespace nioc::logger
