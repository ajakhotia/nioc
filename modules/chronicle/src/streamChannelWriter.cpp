////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "streamChannelWriter.hpp"
#include "utils.hpp"
#include <algorithm>
#include <numeric>
#include <spdlog/spdlog.h>

namespace nioc::chronicle
{
namespace
{

std::filesystem::path setupLogRoot(std::filesystem::path logRoot)
{
  namespace fs = std::filesystem;

  if(fs::exists(logRoot))
  {
    throw std::logic_error(
        "[StreamChannelWriter::StreamChannelWriter] Directory or file" + logRoot.string() +
        " exists already.");
  }

  if(not fs::create_directories(logRoot))
  {
    throw std::runtime_error(
        "[StreamChannelWriter::StreamChannelWriter] Unable to create directory at " +
        logRoot.string() + ".");
  }

  return logRoot;
}

} // namespace

StreamChannelWriter::StreamChannelWriter(
    std::filesystem::path logRoot,
    const std::uint64_t maxFileSizeInBytes):
    mLogRoot(setupLogRoot(std::move(logRoot))),
    mMaxFileSizeInBytes(maxFileSizeInBytes),
    mIndexFile(mLogRoot / kIndexFileName),
    mRollCounter(std::numeric_limits<std::uint64_t>::max()),
    mActiveLogRoll(nextRollFilePath())
{
}

void StreamChannelWriter::writeFrame(const ConstByteSpan& data)
{
  const auto sizeInBytes = data.size_bytes();
  rollCheckAndIndex(sizeInBytes);

  // Write the size and the blob to the current roll.
  ReadWriteUtil<std::span<const std::byte>>::write(mActiveLogRoll, data);

  // Check if the file is still good.
  if(not mActiveLogRoll.good())
  {
    throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
  }
}

void StreamChannelWriter::writeFrame(std::span<const ConstByteSpan> dataCollection)
{
  const auto sizeInBytes = computeTotalSizeInBytes(dataCollection);
  rollCheckAndIndex(sizeInBytes);

  // Write the size and the blob to the current roll.
  std::ranges::for_each(
      dataCollection,
      [this](const auto& data)
      {
        ReadWriteUtil<std::span<const std::byte>>::write(mActiveLogRoll, data);
      });

  // Check if the file is still good.
  if(not mActiveLogRoll.good())
  {
    throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
  }
}

void StreamChannelWriter::rollCheckAndIndex(const std::uint64_t requiredSizeInBytes)
{
  if(requiredSizeInBytes == 0U)
  {
    return;
  }

  // Advance to the next roll if there isn't enough space in the current roll.
  if(not fileHasSpace(mActiveLogRoll, requiredSizeInBytes, mMaxFileSizeInBytes))
  {
    mActiveLogRoll = std::ofstream(nextRollFilePath());
  }

  // Write the index of the roll and the position of the upcoming data blob w.r.t to the start
  // of the roll to the index file.
  if(const auto position = mActiveLogRoll.tellp(); position >= 0)
  {
    // Create and write an IndexEntry to the index file.
    ReadWriteUtil<IndexEntry>::write(
        mIndexFile,
        { .mRollId = mRollCounter,
          .mOffset = static_cast<std::uint64_t>(position),
          .mSize = requiredSizeInBytes });
  }
  else
  {
    throw std::runtime_error("[StreamChannelWriter::index] Unable to retrieve the write position");
  }
}

std::filesystem::path StreamChannelWriter::nextRollFilePath()
{
  return mLogRoot / buildRollName(++mRollCounter);
}


} // namespace nioc::chronicle
