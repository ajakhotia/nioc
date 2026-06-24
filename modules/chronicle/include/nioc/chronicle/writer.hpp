////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "channel.hpp"
#include "crate.hpp"
#include "defines.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <nioc/common/locked.hpp>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/tape.hpp>
#include <span>
#include <unordered_map>

namespace nioc::chronicle
{

/// @brief Records byte frames to a chronicle for later replay.
///
/// Frames from all channels are recorded in one global order, and a @ref Reader replays them in
/// that order. Record through a @ref Channel from @ref channel, or use the @ref write method.
///
/// Thread-safe across channels. A single channel has one producer: record on it from one thread
/// only.
class Writer
{
public:
  /// @brief Default capacity of one roll file.
  static constexpr auto kDefaultRollCapacity = 1024ULL * 1024ULL * 1024ULL;

  /// @brief Default capacity of the timeline file.
  ///
  /// A self-imposed budget on how many frames can be recorded (one @ref TimelineEntry each), traded
  /// against the address space left for channel rolls — not a hardware-derived limit. The file is
  /// sparse, so the reservation costs address space, not disk. Tune it per deployment: raise it for
  /// huge counts of tiny frames, lower it to be frugal.
  static constexpr auto kDefaultTimelineCapacity = 4ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;

  /// @brief Records into @p rootDir.
  ///
  /// @param rootDir An existing empty directory to write into.
  ///
  /// @param rollCapacity Largest size of one roll file. A larger frame still records.
  ///
  /// @param timelineCapacity Size of the timeline file; see @ref kDefaultTimelineCapacity.
  ///
  /// @throws std::invalid_argument If @p rootDir does not exist, is not a directory, or is not
  /// empty.
  explicit Writer(
      std::filesystem::path rootDir,
      std::size_t rollCapacity = kDefaultRollCapacity,
      std::size_t timelineCapacity = kDefaultTimelineCapacity);

  Writer(const Writer&) = delete;

  Writer(Writer&&) noexcept = delete;

  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  /// @brief Returns the channel for @p channelId, creating it on first use.
  ///
  /// The reference stays valid for the writer's lifetime.
  ///
  /// @param channelId Channel to acquire.
  [[nodiscard]] Channel& channel(ChannelId channelId);

  /// @brief Records @p data on a channel by copying it, returning a read-only view of the frame.
  ///
  /// @param channelId Channel to record on.
  ///
  /// @param data Frame bytes.
  ///
  /// @return A crate over the recorded frame.
  Crate write(ChannelId channelId, std::span<const std::byte> data);

  /// @brief Returns the chronicle directory.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  using ChannelMap = std::unordered_map<ChannelId, std::unique_ptr<Channel>>;

  const std::filesystem::path mLogRoot;
  const std::size_t mRollCapacity;
  containers::Tape<containers::MmapArray<TimelineEntry>> mTimeline;
  common::Locked<ChannelMap> mLockedChannelMap;
};

} // namespace nioc::chronicle
