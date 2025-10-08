////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "channelReader.hpp"
#include "memoryCrateImpl.hpp"

namespace nioc::logger
{

namespace
{
constexpr const auto kLogRollBufferSize = 5UL;


} // namespace

ChannelReader::ChannelReader(std::filesystem::path logRoot):
    mLogRoot(validatePath(std::move(logRoot))),
    mIndexFile(mLogRoot / kIndexFileName),
    mLogRollBuffer(kLogRollBufferSize)
{
}

MemoryCrate ChannelReader::read()
{
  const auto indexPtrOffset = mNextReadIndex * sizeof(IndexEntry);

  if(indexPtrOffset >= mIndexFile.size())
  {
    throw std::runtime_error(
        "Reached end of index file at " + (mLogRoot / kIndexFileName).string());
  }

  ++mNextReadIndex;

  const auto index = ReadWriteUtil<IndexEntry>::read(
      std::next(mIndexFile.begin(), static_cast<ssize_t>(indexPtrOffset)));

  auto logRollPtr = acquireLogRoll(index.mRollId);

  return MemoryCrate{
    std::make_shared<MemoryCrate::MemoryCrateImpl>(std::move(logRollPtr), index)
  };
}

ChannelReader::MappedFilePtr ChannelReader::acquireLogRoll(const std::uint64_t rollId)
{
  const auto iter = std::ranges::find_if(
      mLogRollBuffer,
      [rollId](const MappedLogRoll& mappedLogRoll)
      {
        return mappedLogRoll.mRollId == rollId;
      });

  // If the roll doesn't exist, then map it in.
  if(iter == mLogRollBuffer.end())
  {
    auto mappedFilePtr = std::make_shared<MappedFile>(mLogRoot / buildRollName(rollId));
    mLogRollBuffer.push_back({ rollId, mappedFilePtr });
    return mappedFilePtr;
  }

  return iter->mMappedFilePtr;
}

} // namespace nioc::logger
