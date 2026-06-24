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

/// @brief One append-only channel of a chronicle.
///
/// Records byte frames on the channel into a growing run of roll files. A channel has a single
/// producer: call it from one thread only. Acquire it once with @ref Writer::channel; the reference
/// stays valid for the writer's lifetime.
class Channel
{
public:
  /// @brief Creates a channel that records under @p channelDir.
  ///
  /// @param channelId Identity recorded for every frame on this channel.
  ///
  /// @param channelDir Directory for this channel's roll files.
  ///
  /// @param rollCapacity Largest size of one roll file.
  ///
  /// @param timeline The chronicle's ordering, shared across channels; must outlive this channel.
  Channel(
      ChannelId channelId,
      std::filesystem::path channelDir,
      std::size_t rollCapacity,
      containers::Tape<containers::MmapArray<TimelineEntry>>& timeline);

  Channel(const Channel&) = delete;

  Channel(Channel&&) noexcept = delete;

  ~Channel();

  Channel& operator=(const Channel&) = delete;

  Channel& operator=(Channel&&) noexcept = delete;

  /// @brief This channel's identity.
  [[nodiscard]] ChannelId id() const noexcept;

  /// @brief Claims a writable region to build a frame into.
  ///
  /// Fill the returned reservation's @ref Reservation::span, then make a @ref Crate from it to
  /// record the frame. A reservation larger than @p rollCapacity gets its own oversized roll.
  ///
  /// @param size Number of bytes to claim.
  ///
  /// @return A reservation over the claimed region.
  [[nodiscard]] Reservation reserve(std::size_t size);

  /// @brief Records @p data by copying it, returning a read-only view of the frame.
  ///
  /// @param data Frame bytes.
  ///
  /// @return A crate over the recorded frame.
  Crate write(std::span<const std::byte> data);

private:
  friend class Reservation;

  const ChannelId mChannelId;
  const std::filesystem::path mChannelDir;
  const std::size_t mRollCapacity;
  containers::Tape<containers::MmapArray<TimelineEntry>>& mTimeline;
  std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> mActiveRoll;
  std::uint64_t mActiveRollId{0ULL};

  /// @brief Rewinds @p reservation's claim off by trimming it to the @p usedSize. Defaults
  ///        to removing the entire reservation.
  ///
  /// @param reservation The reservation whose claim to release.
  ///
  /// @param usedSize
  ///
  /// @return True if the claim was released; false if a later claim had already stranded it.
  void rewind(const Reservation& reservation, std::size_t usedSize = 0);

  /// @brief Resizes @p reservation to @p newSize bytes, rolling over to a fresh roll if needed.
  ///
  /// Releases @p reservation's claim back to its roll and replaces it in place with a new one of
  /// @p newSize (see @ref reserve), opening a fresh roll when the new size no longer fits. No bytes
  /// are carried over — the previous contents are abandoned, so the caller must copy out anything
  /// it needs first. @p reservation must be the tail of its roll (single-producer use).
  ///
  /// @param reservation The reservation to resize, replaced in place with the new claim.
  ///
  /// @param newSize New writable size in bytes.
  void modify(Reservation& reservation, std::size_t newSize);

  /// @brief Opens a fresh roll of at least @p minCapacity bytes, trimming the retiring one.
  void openNewRoll(std::size_t minCapacity);

  /// @brief Appends a frame's locator to this channel's timeline; called by @ref Crate.
  void append(const TimelineEntry& entry);
};

} // namespace nioc::chronicle
