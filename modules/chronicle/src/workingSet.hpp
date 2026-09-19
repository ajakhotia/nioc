////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "adviceRing.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/defines.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <unordered_map>
#include <vector>

namespace nioc::chronicle
{

/// @brief The set of log pages a replay needs resident around its cursor: it owns the payload
/// rolls' mappings and continuously tells the kernel which pages to load ahead of the cursor and
/// which to release behind it, per the Reader's byte budgets.
///
/// The Reader calls acquire() once per record; everything else is internal. Ahead of the cursor,
/// the working set walks the timeline and asks the kernel to read the next readAheadBytes of
/// payload into the page cache. Behind the cursor, release follows consumption, not the cursor:
/// served records are grouped into strides whose token every Crate from that stride shares, and a
/// stride's pages leave memory and the page cache only once no Crate references it and it has
/// fallen trailBehindBytes behind. A consumer may therefore hold any Crate for any time and never
/// re-fault its bytes from disk; what it holds simply stays resident. Fully released rolls retire
/// their mappings, and the timeline's own pages get the cursor-keyed sliding treatment (the
/// Reader consumes them in place). With prompt consumers resident memory stays near
/// readAheadBytes + trailBehindBytes regardless of log or roll size; retained Crates add exactly
/// the strides they pin, as clean reclaimable pages.
///
/// Advice never affects correctness: a released page transparently re-faults from the immutable
/// roll file, and a Crate's stride token keeps its roll's mapping alive for as long as the Crate
/// does.
///
/// Non-copyable, non-movable. Not thread-safe.
///
/// @see Reader, AdviceRing
class WorkingSet
{
public:
  /// A roll: one memory-mapped chunk of a channel's payload bytes, addressed by byte offset.
  using Roll = containers::MmapConstArray<std::byte>;

  /// The mapped timeline the replay follows.
  using Timeline = containers::MmapConstArray<TimelineEntry>;

  /// @brief Byte budgets for the window kept resident around the replay cursor.
  struct Budget
  {
    /// Payload bytes the kernel is asked to hold ready ahead of the cursor.
    std::uint64_t mReadAheadBytes;

    /// Consumed payload bytes kept resident behind the cursor before release.
    std::uint64_t mTrailBehindBytes;
  };

  /// @brief Bind the working set to a chronicle and start maintaining the resident window.
  ///
  /// @param logRoot The chronicle's root directory; roll files are resolved beneath it.
  ///
  /// @param timeline The mapped timeline driving replay; borrowed, must outlive the working set.
  ///
  /// @param budget The window's byte budgets.
  WorkingSet(std::filesystem::path logRoot, const Timeline& timeline, Budget budget);

  WorkingSet(const WorkingSet&) = delete;

  WorkingSet(WorkingSet&&) noexcept = delete;

  /// @brief Drop every mapping the working set still holds. Mappings shared with live Crates
  /// survive through their shared ownership.
  ~WorkingSet();

  WorkingSet& operator=(const WorkingSet&) = delete;

  WorkingSet& operator=(WorkingSet&&) noexcept = delete;

  /// @brief Serve the record at @p record as a Crate and advance the resident window.
  ///
  /// Called once per record, in timeline order: @p record must be non-decreasing across calls.
  /// Maps the record's roll on first use; the Crate shares its stride's token, which defers the
  /// stride's release until every Crate from it is gone.
  ///
  /// @param record The record to serve; a dereferenceable iterator of the timeline.
  ///
  /// @throws std::runtime_error if the record's roll file cannot be opened or mapped.
  [[nodiscard]] Crate acquire(Timeline::const_iterator record);

private:
  /// One channel's live rolls keyed by roll id, so oldest first: they span the trim edge to the
  /// prefetch edge. The prefetch walk maps a roll as it reaches it (address space only; pages
  /// arrive by advice), the cursor serves from it, and it retires once the trim edge passes it.
  using Rolls = std::map<std::uint64_t, std::shared_ptr<const Roll>>;

  /// @brief One run of consecutively served records, the granularity of release. Every Crate
  /// from the run shares a pointer to it as its token; the run's pages are released only once the
  /// working set holds that pointer alone.
  struct Stride
  {
    /// One past the run's last record.
    Timeline::const_iterator mEnd;

    /// The run's payload bytes.
    std::uint64_t mBytes{0ULL};

    /// The rolls the run's records live in, held so every Crate's bytes outlive its stride.
    std::vector<std::shared_ptr<const Roll>> mRolls;
  };

  /// A batch of contiguous payload ranges keyed by (channel, roll), coalesced from one window
  /// walk so advice is issued per roll range rather than per record.
  using RangeBatch = std::unordered_map<ChannelId, std::map<std::uint64_t, FileRange>>;

  /// The chronicle's root directory; roll paths are derived from it.
  const std::filesystem::path mLogRoot;

  /// The mapped timeline; borrowed from the Reader.
  const Timeline& mTimeline;

  /// The window's byte budgets.
  const Budget mBudget;

  /// The advice channel to the kernel: asynchronous through io_uring where available, synchronous
  /// otherwise.
  AdviceRing mRing;

  /// Every channel's live rolls, created on first use of each channel.
  std::unordered_map<ChannelId, Rolls> mChannels;

  /// One past the last prefetched record; never behind the cursor.
  Timeline::const_iterator mPrefetchEdge;

  /// Payload bytes advised ahead of the cursor, i.e. the filled part of the read-ahead window.
  std::uint64_t mPrefetchedBytes{0ULL};

  /// The oldest record whose pages have not been released yet.
  Timeline::const_iterator mTrimEdge;

  /// Consumed payload bytes not yet released, i.e. the distance from the trim edge to the cursor.
  std::uint64_t mPendingTrimBytes{0ULL};

  /// The stride currently being served; null before the first record and right after a close.
  std::shared_ptr<Stride> mOpenStride;

  /// Closed strides awaiting release, oldest first; the front releases once its token is unique
  /// and it has fallen the trail-behind distance back.
  std::deque<std::shared_ptr<const Stride>> mClosedStrides;

  /// Timeline bytes already released by the timeline's own trailing trim.
  std::uint64_t mTimelineTrimmedBytes{0ULL};

  /// @brief The mapping of a channel's roll, created and remembered on first use.
  ///
  /// @param channelId The channel's id, used to derive the roll path.
  ///
  /// @throws std::runtime_error if the roll file cannot be opened or mapped.
  const std::shared_ptr<const Roll>& roll(Rolls& rolls, ChannelId channelId, std::uint64_t rollId);

  /// @brief Add @p record, served from @p roll, to the open stride and return the stride's token
  /// for the record's Crate, closing the stride into the release queue once it reaches the
  /// stride size.
  std::shared_ptr<const Stride> joinStride(
      const std::shared_ptr<const Roll>& roll,
      Timeline::const_iterator record);

  /// @brief Grow @p batch to cover @p entry's payload bytes.
  static void cover(RangeBatch& batch, const TimelineEntry& entry);

  /// @brief Walk the timeline forward from the prefetch edge and advise the kernel until the
  /// read-ahead window is full, mapping rolls as the walk reaches them. The window always covers
  /// @p record.
  void extendWindow(Timeline::const_iterator record);

  /// @brief Release the oldest strides that no Crate references once they fall the trail-behind
  /// distance behind, retiring rolls the trim edge has fully passed.
  void advanceTrim();

  /// @brief Release consumed timeline pages in coarse strides behind @p record.
  void trimTimeline(Timeline::const_iterator record);

  /// @brief Ask the kernel to load @p range of @p roll into the page cache.
  void prefetch(const Roll& roll, FileRange range);

  /// @brief Release @p range of @p roll from memory and page cache: page-table eviction through
  /// the mapping first, then the page-cache release through the file.
  void release(const Roll& roll, FileRange range);
};

} // namespace nioc::chronicle
