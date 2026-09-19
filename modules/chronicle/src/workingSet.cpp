////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "workingSet.hpp"

#include "utils.hpp"
#include <algorithm>
#include <fcntl.h>
#include <iterator>
#include <nioc/common/utils.hpp>
#include <span>
#include <unistd.h>
#include <utility>

namespace nioc::chronicle
{
namespace
{

/// Hysteresis for the forward walk: the window refills once it has drained by this much, so ring
/// submissions happen in batches instead of per record.
constexpr auto kRefillBytes = std::uint64_t{8ULL * 1024ULL * 1024ULL};

/// Release granularity: served records are grouped into strides of this many payload bytes, and
/// a stride's pages are released as one unit once no Crate references it.
constexpr auto kStrideBytes = std::uint64_t{8ULL * 1024ULL * 1024ULL};

/// The timeline's own trailing trim advances in strides this large ...
constexpr auto kTimelineTrimStrideBytes = std::uint64_t{1024ULL * 1024ULL};

/// ... while keeping this many recent timeline bytes resident behind the cursor.
constexpr auto kTimelineKeepBytes = std::uint64_t{256ULL * 1024ULL};

/// Submission slots in the advice ring; at 128 KiB per WILLNEED this covers a 64 MiB burst per
/// submit call, and the ring loops submission when a walk queues more.
constexpr auto kRingEntryCount = std::uint32_t{512U};

/// @brief The path of one roll file within the chronicle.
std::filesystem::path rollPath(
    const std::filesystem::path& logRoot,
    const ChannelId channelId,
    const std::uint64_t rollId)
{
  return logRoot / common::hexString(channelId.mValue) / buildRollName(rollId);
}

/// @brief @p minuend - @p subtrahend, clamped at zero.
constexpr std::uint64_t saturatingSubtract(
    const std::uint64_t minuend,
    const std::uint64_t subtrahend) noexcept
{
  return minuend - std::min(minuend, subtrahend);
}

} // namespace

WorkingSet::WorkingSet(
    std::filesystem::path logRoot,
    const Timeline& timeline,
    const Budget budget):
  mLogRoot{std::move(logRoot)},
  mTimeline{timeline},
  mBudget{budget},
  mRing{kRingEntryCount},
  mPrefetchEdge{mTimeline.begin()},
  mTrimEdge{mTimeline.begin()}
{
}

WorkingSet::~WorkingSet() = default;

Crate WorkingSet::acquire(const Timeline::const_iterator record)
{
  extendWindow(record);

  auto& rolls = mChannels[record->mChannelId];
  const auto& mapped = roll(rolls, record->mChannelId, record->mRollId);
  const auto span = std::span{*mapped}.subspan(record->mOffset, record->mSize);
  auto crate = Crate{joinStride(mapped, record), span};

  mPrefetchedBytes = saturatingSubtract(mPrefetchedBytes, record->mSize);
  mPendingTrimBytes += record->mSize;
  advanceTrim();
  trimTimeline(record);

  return crate;
}

const std::shared_ptr<const WorkingSet::Roll>& WorkingSet::roll(
    Rolls& rolls,
    const ChannelId channelId,
    const std::uint64_t rollId)
{
  auto [mapped, inserted] = rolls.try_emplace(rollId);
  if(inserted)
  {
    mapped->second = std::make_shared<const Roll>(rollPath(mLogRoot, channelId, rollId));
  }
  return mapped->second;
}

std::shared_ptr<const WorkingSet::Stride> WorkingSet::joinStride(
    const std::shared_ptr<const Roll>& roll,
    const Timeline::const_iterator record)
{
  if(not mOpenStride)
  {
    mOpenStride = std::make_shared<Stride>();
  }
  auto& stride = *mOpenStride;
  if(stride.mRolls.empty() or stride.mRolls.back() != roll)
  {
    stride.mRolls.push_back(roll);
  }
  stride.mEnd = std::next(record);
  stride.mBytes += record->mSize;

  auto token = std::shared_ptr<const Stride>{mOpenStride};
  if(stride.mBytes >= kStrideBytes)
  {
    mClosedStrides.push_back(std::move(mOpenStride));
  }
  return token;
}

void WorkingSet::cover(RangeBatch& batch, const TimelineEntry& entry)
{
  // Within one roll the walked records sit back to back (offsets ascend), so the batch keeps one
  // contiguous range per roll.
  batch[entry.mChannelId]
      .try_emplace(entry.mRollId, FileRange{.mBegin = entry.mOffset, .mEnd = entry.mOffset})
      .first->second.cover(entry.mOffset, entry.mSize);
}

void WorkingSet::extendWindow(const Timeline::const_iterator record)
{
  mPrefetchEdge = std::max(mPrefetchEdge, record);

  const auto deficit = saturatingSubtract(mBudget.mReadAheadBytes, mPrefetchedBytes);
  const auto recordCovered = mPrefetchEdge > record;
  const auto refillDue = deficit >= kRefillBytes;
  if(recordCovered and not refillDue)
  {
    return;
  }

  auto batch = RangeBatch{};
  while(mPrefetchEdge != mTimeline.end() and
        (mPrefetchedBytes < mBudget.mReadAheadBytes or mPrefetchEdge <= record))
  {
    cover(batch, *mPrefetchEdge);
    mPrefetchedBytes += mPrefetchEdge->mSize;
    ++mPrefetchEdge;
  }

  for(const auto& [channelId, rollRanges]: batch)
  {
    auto& rolls = mChannels[channelId];
    for(const auto& [rollId, range]: rollRanges)
    {
      prefetch(*roll(rolls, channelId, rollId), range);
    }
  }
  mRing.submit();
}

void WorkingSet::advanceTrim()
{
  auto batch = RangeBatch{};
  while(not mClosedStrides.empty() and
        mClosedStrides.front().use_count() == 1 and
        mPendingTrimBytes >= mClosedStrides.front()->mBytes + mBudget.mTrailBehindBytes)
  {
    for(; mTrimEdge != mClosedStrides.front()->mEnd; ++mTrimEdge)
    {
      cover(batch, *mTrimEdge);
      mPendingTrimBytes = saturatingSubtract(mPendingTrimBytes, mTrimEdge->mSize);
    }
    mClosedStrides.pop_front();
  }

  for(const auto& [channelId, rollRanges]: batch)
  {
    auto& rolls = mChannels.at(channelId);
    for(const auto& [rollId, range]: rollRanges)
    {
      if(const auto mapped = rolls.find(rollId); mapped != rolls.end())
      {
        release(*mapped->second, range);
      }
    }

    // Every roll older than the newest trimmed one has no further reader: rolls within a channel
    // are consumed in order, and the trim edge has passed them.
    rolls.erase(rolls.begin(), rolls.lower_bound(rollRanges.rbegin()->first));
  }
  if(not batch.empty())
  {
    mRing.submit();
  }
}

void WorkingSet::trimTimeline(const Timeline::const_iterator record)
{
  const auto consumedBytes = static_cast<std::uint64_t>(std::distance(mTimeline.begin(), record)) *
                             sizeof(TimelineEntry);
  const auto keepEdge = saturatingSubtract(consumedBytes, kTimelineKeepBytes);
  if(keepEdge < mTimelineTrimmedBytes + kTimelineTrimStrideBytes)
  {
    return;
  }

  mTimeline.evict(
      std::next(
          mTimeline.begin(),
          static_cast<std::ptrdiff_t>(mTimelineTrimmedBytes / sizeof(TimelineEntry))),
      std::next(mTimeline.begin(), static_cast<std::ptrdiff_t>(keepEdge / sizeof(TimelineEntry))));
  mRing.advise(
      mTimeline.nativeHandle(),
      FileRange{.mBegin = mTimelineTrimmedBytes, .mEnd = keepEdge},
      POSIX_FADV_DONTNEED);
  mRing.submit();
  mTimelineTrimmedBytes = keepEdge;
}

void WorkingSet::prefetch(const Roll& roll, const FileRange range)
{
  mRing.advise(roll.nativeHandle(), range, POSIX_FADV_WILLNEED);
}

void WorkingSet::release(const Roll& roll, const FileRange range)
{
  // Page-table eviction first: fadvise skips any page a live mapping still references, so the
  // mapping's own eviction must precede it for the pages to actually leave memory.
  roll.evict(
      std::next(roll.begin(), static_cast<std::ptrdiff_t>(range.mBegin)),
      std::next(roll.begin(), static_cast<std::ptrdiff_t>(range.mEnd)));
  mRing.advise(roll.nativeHandle(), range, POSIX_FADV_DONTNEED);
}

} // namespace nioc::chronicle
