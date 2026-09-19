////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include "workingSet.hpp"
#include <cassert>
#include <iterator>
#include <nioc/chronicle/reader.hpp>
#include <nioc/common/filesystem.hpp>
#include <optional>
#include <utility>

namespace nioc::chronicle
{

const Entry& Reader::Iterator::operator*() const noexcept
{
  assert(mEntry.has_value());
  return *mEntry; // NOLINT(bugprone-unchecked-optional-access)
}

const Entry* Reader::Iterator::operator->() const noexcept
{
  assert(mEntry.has_value());
  return &*mEntry; // NOLINT(bugprone-unchecked-optional-access)
}

Reader::Iterator& Reader::Iterator::operator++()
{
  mEntry = mReader->readNextEntry();
  return *this;
}

void Reader::Iterator::operator++(int)
{
  ++*this;
}

bool Reader::Iterator::operator==(const std::default_sentinel_t /*end*/) const noexcept
{
  return not mEntry.has_value();
}

Reader::Iterator::Iterator(Reader& reader): mReader{&reader}, mEntry{mReader->readNextEntry()} {}

Reader::Iterator Reader::begin()
{
  return Iterator{*this};
}

std::default_sentinel_t Reader::end() noexcept
{
  return {};
}

Reader::Reader(
    std::filesystem::path logRoot,
    const std::uint64_t readAheadBytes,
    const std::uint64_t trailBehindBytes):
  mLogRoot{common::requireExistingDirectory(std::move(logRoot))},
  mTimelineFile{common::requireExistingFile(mLogRoot / kTimelineFileName)},
  mWorkingSet{std::make_unique<WorkingSet>(
      mLogRoot,
      mTimelineFile,
      WorkingSet::Budget{.mReadAheadBytes = readAheadBytes, .mTrailBehindBytes = trailBehindBytes})}
{
}

Reader::~Reader() = default;

std::optional<Entry> Reader::readNextEntry()
{
  if(mTimelineCursor == mTimelineFile.end())
  {
    return std::nullopt;
  }

  const auto channelId = mTimelineCursor->mChannelId;
  auto crate = mWorkingSet->acquire(mTimelineCursor);
  ++mTimelineCursor;

  return Entry{.mChannelId = channelId, .mCrate = std::move(crate)};
}

} // namespace nioc::chronicle
