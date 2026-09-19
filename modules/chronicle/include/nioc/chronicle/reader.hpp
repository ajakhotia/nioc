////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "crate.hpp"
#include "defines.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <memory>
#include <nioc/containers/mmapConstArray.hpp>
#include <optional>

namespace nioc::chronicle
{

class WorkingSet;

/// @brief One replayed record produced by a Reader: the channel it was logged on, paired with its
/// payload bytes.
///
/// @see Reader, Crate, ChannelId
struct Entry
{
  /// The channel this record was logged on.
  ChannelId mChannelId;

  /// The record's payload bytes. The crate holds its backing roll alive, so these bytes stay valid
  /// even after the iterator advances past this entry or the Reader is destroyed.
  Crate mCrate;
};

/// @brief A single-pass input range over a chronicle log directory that yields its records in
/// recorded timeline order.
///
/// Example:
///
///     nioc::chronicle::Reader reader{"/path/to/log"};
///     for(const auto& entry: reader)
///     {
///       process(entry.mChannelId, entry.mCrate.span());
///     }
///
/// Iterate the reader exactly once, with a range-based for loop or begin()/end(). The pass is
/// destructive and cannot be restarted: construct a fresh Reader to replay again. Non-copyable and
/// non-movable; pin it in place (on the stack or behind a pointer). Not thread-safe.
///
/// @see Entry, Iterator
class Reader
{
public:
  /// Default read-ahead window: generous against disk-latency bursts, small against machine RAM.
  static constexpr auto kDefaultReadAheadBytes = std::uint64_t{256ULL * 1024ULL * 1024ULL};

  /// Default trail-behind distance: grace for consumers that lag slightly behind the cursor.
  static constexpr auto kDefaultTrailBehindBytes = std::uint64_t{64ULL * 1024ULL * 1024ULL};

  /// @brief Open the chronicle rooted at @p logRoot for replay.
  ///
  /// The Reader keeps a bounded working set resident around its cursor: the next @p readAheadBytes
  /// of payload are loaded ahead of consumption, and consumed pages are released once they fall
  /// @p trailBehindBytes behind, so resident memory stays near the two budgets' sum regardless of
  /// log size. Size the read-ahead to cover the burstiest stretch of disk latency times
  /// consumption rate, and the sum to fit the machine's free memory. A consumer lagging within the
  /// trail-behind distance re-reads nothing; one reaching further back (a retained Crate) stays
  /// correct and transparently re-reads released pages from the log.
  ///
  /// @param logRoot Path to the chronicle's root directory; must name an existing directory that
  /// holds a timeline file. A chronicle that recorded nothing (an empty timeline) replays as an
  /// empty range.
  ///
  /// @param readAheadBytes Payload bytes the kernel is asked to hold ready ahead of the cursor.
  ///
  /// @param trailBehindBytes Consumed payload bytes kept resident behind the cursor before release.
  ///
  /// @throws std::invalid_argument If @p logRoot is not a directory or holds no timeline file.
  ///
  /// @throws std::runtime_error If the timeline file cannot be mapped.
  explicit Reader(
      std::filesystem::path logRoot,
      std::uint64_t readAheadBytes = kDefaultReadAheadBytes,
      std::uint64_t trailBehindBytes = kDefaultTrailBehindBytes);

  Reader(const Reader&) = delete;

  Reader(Reader&&) noexcept = delete;

  /// @brief End replay and drop the Reader's own hold on its memory-mapped files. Mappings
  /// shared with still-live Crates survive through their shared ownership and are released when
  /// the last such Crate is destroyed.
  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&&) noexcept = delete;

  /// @brief A single-pass input iterator over the parent Reader's timeline.
  ///
  /// Obtain one from Reader::begin() and compare it against Reader::end() to detect exhaustion. The
  /// entry it refers to stays valid only until the next increment.
  ///
  /// @see Reader::begin, Reader::end
  class Iterator
  {
  public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = Entry;
    using difference_type = std::ptrdiff_t;

    /// @brief Construct a past-the-end iterator, equal to Reader::end() and not dereferenceable.
    Iterator() = default;

    /// @brief The entry at the current position.
    ///
    /// Undefined behavior if the iterator is exhausted (equals Reader::end()).
    [[nodiscard]] const Entry& operator*() const noexcept;

    /// @brief Member access to the entry at the current position.
    ///
    /// Undefined behavior if the iterator is exhausted (equals Reader::end()).
    [[nodiscard]] const Entry* operator->() const noexcept;

    /// @brief Advance to the next record, loading it from the log.
    ///
    /// Invalidates the previously referenced entry. Once the timeline is exhausted, the iterator
    /// compares equal to Reader::end().
    Iterator& operator++();

    /// @brief Advance past the current record, discarding it.
    void operator++(int);

    /// @brief Test whether the iterator has reached the end sentinel.
    ///
    /// @return True once the timeline is exhausted.
    [[nodiscard]] bool operator==(std::default_sentinel_t end) const noexcept;

  private:
    friend class Reader;

    explicit Iterator(Reader& reader);

    Reader* mReader{nullptr};
    std::optional<Entry> mEntry;
  };

  /// @brief Start replay, returning an iterator at the first recorded entry (or at end() if the log
  /// is empty).
  ///
  /// Single-pass: call once per Reader. It consumes one record to position the cursor and resumes
  /// from wherever the Reader left off, so calling it again after iterating advances further rather
  /// than rewinding to the start.
  ///
  /// @see end
  [[nodiscard]] Iterator begin();

  /// @brief The end sentinel for the replay range.
  [[nodiscard]] static std::default_sentinel_t end() noexcept;

private:
  /// The memory-mapped timeline: the ordered list of records to replay, one TimelineEntry each,
  /// naming the channel, roll, and offset where every record's bytes live.
  using TimelineFile = containers::MmapConstArray<TimelineEntry>;

  /// The chronicle's root directory, captured at construction.
  const std::filesystem::path mLogRoot;

  /// The mapped timeline driving replay; empty for a chronicle that recorded nothing.
  const TimelineFile mTimelineFile;

  /// The working set: owns the payload rolls' mappings and keeps the kernel loading pages ahead
  /// of the cursor and releasing them behind it, per the construction-time byte budgets.
  /// Never null; destroyed before mTimelineFile, which it borrows.
  const std::unique_ptr<WorkingSet> mWorkingSet;

  /// The next timeline record to read; end() once the replay is exhausted.
  TimelineFile::const_iterator mTimelineCursor{mTimelineFile.begin()};

  /// @brief Read the record at the current cursor and advance the cursor by one.
  ///
  /// Called by the Iterator on construction and on each increment.
  ///
  /// @return The next Entry, or std::nullopt once the timeline is exhausted.
  std::optional<Entry> readNextEntry();
};

} // namespace nioc::chronicle
