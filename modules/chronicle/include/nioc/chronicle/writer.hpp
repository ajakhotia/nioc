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
#include <span>
#include <unordered_map>

namespace nioc::chronicle
{

class Timeline;

/// @brief Records byte frames to a chronicle for later replay.
///
/// Frames from all channels are recorded in one global order, and a @ref Reader replays them in
/// that order. Record through a @ref Channel from @ref channel, or use @ref write for a one-off
/// frame.
///
/// Thread-safe across channels. A single channel has one producer: record on it from one thread
/// only.
class Writer
{
public:
  /// @brief Default capacity of one roll file.
  static constexpr auto kDefaultRollCapacity = 1024ULL * 1024ULL * 1024ULL;

  /// @brief Default capacity of one timeline file; the timeline grows by adding files.
  static constexpr auto kDefaultTimelineFileCapacity = 64ULL * 1024ULL * 1024ULL;

  /// @brief Records into @p rootDir.
  ///
  /// @param rootDir An existing empty directory to write into.
  ///
  /// @param rollCapacity Largest size of one roll file. A larger frame still records.
  ///
  /// @param timelineFileCapacity Largest size of one timeline file.
  ///
  /// @throws std::invalid_argument If @p rootDir does not exist, is not a directory, or is not
  /// empty.
  explicit Writer(
      std::filesystem::path rootDir,
      std::size_t rollCapacity = kDefaultRollCapacity,
      std::size_t timelineFileCapacity = kDefaultTimelineFileCapacity);

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
  const std::unique_ptr<Timeline> mTimeline;
  common::Locked<ChannelMap> mLockedChannelMap;
};

} // namespace nioc::chronicle
