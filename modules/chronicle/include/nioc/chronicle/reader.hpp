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
#include <unordered_map>

namespace nioc::chronicle
{

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
  /// @brief Open the chronicle rooted at @p logRoot for replay.
  ///
  /// @param logRoot Path to the chronicle's root directory; must name an existing directory. A
  /// chronicle whose timeline file is missing or empty replays as an empty range.
  ///
  /// @throws std::invalid_argument If @p logRoot does not exist or is not a directory.
  explicit Reader(std::filesystem::path logRoot);

  Reader(const Reader&) = delete;

  Reader(Reader&&) noexcept = delete;

  /// @brief Close the chronicle and release every memory-mapped file the Reader opened.
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

  /// A roll: one memory-mapped chunk of a channel's payload bytes, addressed by byte offset.
  using Roll = containers::MmapConstArray<std::byte>;

  /// The set of currently mapped rolls for one channel, keyed by roll id. Each value is a weak
  /// reference, so a roll stays mapped only while some live Entry still holds it.
  using RollCache = std::unordered_map<std::uint64_t, std::weak_ptr<const Roll>>;

  /// The chronicle's root directory, captured at construction.
  const std::filesystem::path mLogRoot;

  /// The mapped timeline driving replay, or empty if the chronicle has no timeline file.
  std::unique_ptr<const TimelineFile> mTimelineFile;

  /// The index of the next timeline record to read; the replay cursor into mTimelineFile.
  std::uint64_t mEntryInTimeline{0ULL};

  /// The cache of mapped rolls, partitioned by channel, that keeps recently used rolls mapped so
  /// successive records on the same channel reuse one mapping.
  std::unordered_map<ChannelId, RollCache> mRollCache;

  /// @brief Read the record at the current cursor and advance the cursor by one.
  ///
  /// Called by the Iterator on construction and on each increment.
  ///
  /// @return The next Entry, or std::nullopt once the timeline is exhausted.
  std::optional<Entry> readNextEntry();

  /// @brief Return the roll holding the given channel's bytes, mapping it if it is not already
  /// resident and recording it in mRollCache.
  ///
  /// @param channelId The channel whose roll is needed.
  ///
  /// @param rollId The id of the roll to map within that channel.
  ///
  /// @return A shared owner of the mapped roll, kept alive by every Entry that references it.
  std::shared_ptr<const Roll> acquireRoll(ChannelId channelId, std::uint64_t rollId);
};

} // namespace nioc::chronicle
