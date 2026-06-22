////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "crate.hpp"
#include "defines.hpp"
#include <cassert>
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

/// @brief One frame read from a chronicle: its channel and its bytes.
struct Entry
{
  ChannelId mChannelId;
  Crate mCrate;
};

/// @brief Replays a chronicle's frames in the order they were recorded.
///
/// A single-pass input range: iterate it once with a range-for, or pass it to a standard algorithm.
///
/// @code
/// for(const auto& entry: Reader{logRoot})
/// {
///   use(entry.mChannelId, entry.mCrate);
/// }
/// @endcode
///
/// Each frame's bytes stay mapped only while a @ref Crate over them lives; copy the crate out of an
/// entry to keep its bytes past the next step. Not thread-safe: use from one thread only.
class Reader
{
public:
  /// @brief Opens the chronicle in @p logRoot for reading.
  ///
  /// @param logRoot Path to the chronicle directory.
  ///
  /// @throws std::invalid_argument If @p logRoot does not exist or is not a directory.
  explicit Reader(std::filesystem::path logRoot);

  Reader(const Reader&) = delete;

  Reader(Reader&&) noexcept = delete;

  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&&) noexcept = delete;

  /// @brief A single-pass input iterator over a chronicle's frames.
  ///
  /// Reads the frame it points at from the owning @ref Reader; advancing it reads the next. All
  /// iterators over one reader share that reader's position, so iterate a reader only once.
  class Iterator
  {
  public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = Entry;
    using difference_type = std::ptrdiff_t;

    /// @brief Constructs a past-the-end iterator.
    Iterator() = default;

    [[nodiscard]] const Entry& operator*() const noexcept
    {
      assert(mEntry.has_value());
      return *mEntry;
    }

    [[nodiscard]] const Entry* operator->() const noexcept
    {
      assert(mEntry.has_value());
      return &*mEntry;
    }

    Iterator& operator++()
    {
      mEntry = mReader->readNextEntry();
      return *this;
    }

    void operator++(int)
    {
      ++*this;
    }

    [[nodiscard]] bool operator==(std::default_sentinel_t /*end*/) const noexcept
    {
      return not mEntry.has_value();
    }

  private:
    friend class Reader;

    explicit Iterator(Reader& reader): mReader{&reader}, mEntry{mReader->readNextEntry()} {}

    Reader* mReader{nullptr};
    std::optional<Entry> mEntry;
  };

  /// @brief Returns an iterator to the first frame, reading it.
  [[nodiscard]] Iterator begin()
  {
    return Iterator{*this};
  }

  /// @brief Returns the past-the-end sentinel.
  [[nodiscard]] static std::default_sentinel_t end() noexcept
  {
    return {};
  }

private:
  using TimelineFile = containers::MmapConstArray<TimelineEntry>;
  using Roll = containers::MmapConstArray<std::byte>;
  using RollCache = std::unordered_map<std::uint64_t, std::weak_ptr<const Roll>>;

  const std::filesystem::path mLogRoot;
  std::unique_ptr<const TimelineFile> mTimelineFile;
  std::uint64_t mEntryInTimeline{0ULL};
  std::unordered_map<ChannelId, RollCache> mRollCache;

  /// @brief Reads the next frame, or nothing once the chronicle is exhausted.
  std::optional<Entry> readNextEntry();

  /// @brief Returns the roll for @p rollId on @p channelId, mapping it if it is not still cached.
  std::shared_ptr<const Roll> acquireRoll(ChannelId channelId, std::uint64_t rollId);
};

} // namespace nioc::chronicle
