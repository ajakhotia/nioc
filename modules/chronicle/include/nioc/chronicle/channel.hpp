////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "crate.hpp"
#include "defines.hpp"
#include "reservation.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/tape.hpp>
#include <span>

namespace nioc::chronicle
{

/// @brief An append-only byte log for a single stream of data, a.k.a. a Channel: stores each
/// record's bytes and indexes it on a shared timeline.
///
/// A Channel owns a directory of rolls (memory-mapped data files). It appends record bytes to the
/// active roll, seals that roll and opens a fresh one once it fills, and records the location of
/// each committed record on a timeline shared with readers. Write a record in one call with
/// write(), or in stages: reserve() space, fill the returned span, then commit.
///
/// Not copyable and not movable: pass it by reference, never by value. Not thread-safe; serialize
/// all access to a given Channel yourself. A Writer creates and owns its Channels; do not construct
/// one standalone.
///
/// @see Reservation, Crate, Writer
class Channel
{
public:
  /// @brief Bind a channel to its on-disk roll directory and a shared timeline.
  ///
  /// No directory or file is touched here; the directory is created on the first reserve().
  ///
  /// @param channelId Identity stamped onto every record written through this channel.
  ///
  /// @param channelDir Directory that holds this channel's rolls. Copied and stored.
  ///
  /// @param rollCapacity Bytes each new roll is sized to. A record larger than this grows its own
  /// roll to fit.
  ///
  /// @param timeline Shared record index. Must outlive this channel; each commit appends to
  /// it.
  Channel(
      ChannelId channelId,
      std::filesystem::path channelDir,
      std::size_t rollCapacity,
      containers::Tape<containers::MmapArray<TimelineEntry>>& timeline);

  Channel(const Channel&) = delete;

  Channel(Channel&&) noexcept = delete;

  /// @brief Seal the active roll, trimming its data file down to the bytes actually written.
  ~Channel();

  Channel& operator=(const Channel&) = delete;

  Channel& operator=(Channel&&) noexcept = delete;

  /// @brief This channel's identity, as supplied at construction.
  [[nodiscard]] ChannelId id() const noexcept;

  /// @brief Reserve a writable byte span for one record, to be filled and then committed.
  ///
  /// Rounds @p size up to a word boundary and carves that many bytes from the active roll, opening
  /// a fresh roll first when the current one cannot fit. The returned Reservation borrows space in
  /// the channel: fill its span(), then commit it to publish the record, or drop it to release the
  /// space. Each outstanding reservation keeps its roll alive. Creates the channel directory on its
  /// first call.
  ///
  /// @param size Minimum writable bytes needed. The returned span may be larger after rounding.
  [[nodiscard]] Reservation reserve(std::size_t size);

  /// @brief Append @p data as a single record and return a Crate viewing the stored bytes.
  ///
  /// Shorthand for reserve(), copy into the span, then commit. The returned Crate keeps the
  /// underlying roll alive for as long as it lives.
  ///
  /// @throws std::runtime_error If the shared timeline is full.
  Crate write(std::span<const std::byte> data);

private:
  friend class Reservation;

  const ChannelId mChannelId;
  const std::filesystem::path mChannelDir;
  const std::size_t mRollCapacity;
  containers::Tape<containers::MmapArray<TimelineEntry>>& mTimeline;
  std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> mActiveRoll;
  std::uint64_t mActiveRollId{0ULL};

  /// @brief Return the unused tail of @p reservation's span to the active roll, keeping only its
  /// first @p usedSize bytes.
  ///
  /// Only the roll's most recent claim can shrink; if the roll declines (e.g. another reservation
  /// has since claimed space past this one), the span stays at full size and a warning is logged
  /// rather than throwing. Called when a Reservation is dropped (@p usedSize 0, releasing all) and
  /// internally by modify() to shrink in place.
  ///
  /// @param reservation The reservation whose span is being trimmed.
  ///
  /// @param usedSize Bytes to keep from the span's front; 0 (the default) releases all of it.
  void rewind(const Reservation& reservation, std::size_t usedSize = 0);

  /// @brief Resize @p reservation to @p newSize bytes, updating it in place.
  ///
  /// Shrinks within the existing span when @p newSize fits (via rewind); otherwise releases the old
  /// span and reseats @p reservation onto a fresh reservation of @p newSize, which may open a new
  /// roll. The handle is rebound either way, so any span() taken earlier is invalidated.
  ///
  /// @param reservation Reservation to resize; replaced in place when growth needs new space.
  ///
  /// @param newSize The reservation's new byte count.
  ///
  /// @see rewind, reserve
  void modify(Reservation& reservation, std::size_t newSize);

  /// @brief Seal the active roll and install a fresh one to append into.
  ///
  /// Shrinks the current roll to its written bytes and bumps the roll id before allocating the
  /// replacement, which is sized to the larger of the channel's roll capacity and @p minCapacity so
  /// an oversized record still fits in a roll of its own.
  ///
  /// @param minCapacity Smallest byte capacity the new roll must have.
  void openNewRoll(std::size_t minCapacity);

  /// @brief Record @p entry on the shared timeline, indexing one committed record.
  ///
  /// @param entry Timeline entry locating the just-committed record.
  ///
  /// @throws std::runtime_error If the shared timeline is at capacity.
  void append(const TimelineEntry& entry);
};

} // namespace nioc::chronicle
