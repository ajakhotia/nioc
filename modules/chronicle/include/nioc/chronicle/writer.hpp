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

/// @brief Records a multi-channel log to a directory on disk, where readers can later replay every
/// channel in write order.
///
/// A chronicle is a root directory holding one shared timeline file plus one subdirectory of
/// memory-mapped data rolls per channel. Each write copies the payload into a channel's roll and
/// stamps the timeline with that record's location, so the global write order is preserved across
/// channels. Channels are created on demand and owned by the Writer for its whole lifetime.
///
/// Example:
///
///     nioc::chronicle::Writer writer{"/data/run42"}; // directory must exist and be empty
///     const auto id = nioc::chronicle::makeChannelId(typeId, "/imu");
///     writer.write(id, payloadBytes);
///
/// Use one Writer per chronicle directory. Not copyable or movable.
///
/// @see Channel, Reader, makeChannelId
class Writer
{
public:
  /// Default byte size of each data roll. A channel seals the active roll and opens a fresh one
  /// once it fills; a single record larger than this grows its roll to fit.
  static constexpr auto kDefaultRollCapacity = 1024ULL * 1024ULL * 1024ULL;

  /// @brief Default byte size of the timeline file.
  ///
  /// Caps the total number of records across all channels: the usable entry count is this value
  /// divided by sizeof(TimelineEntry).
  static constexpr auto kDefaultTimelineCapacity = 4ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;

  /// @brief Open a new chronicle under @p rootDir, allocating the timeline file immediately.
  ///
  /// @param rootDir Chronicle root. Must name an existing, empty directory.
  ///
  /// @param rollCapacity Byte size of each channel's data roll.
  ///
  /// @param timelineCapacity Byte size of the timeline file; bounds the total record count (see
  /// kDefaultTimelineCapacity).
  ///
  /// @throws std::invalid_argument if @p rootDir does not exist, is not a directory, or is not
  /// empty.
  ///
  /// @throws std::filesystem::filesystem_error if a filesystem status query on @p rootDir fails
  /// (for example, a permission error).
  explicit Writer(
      std::filesystem::path rootDir,
      std::size_t rollCapacity = kDefaultRollCapacity,
      std::size_t timelineCapacity = kDefaultTimelineCapacity);

  Writer(const Writer&) = delete;

  Writer(Writer&&) noexcept = delete;

  /// Trim the timeline file down to the bytes actually written.
  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  /// @brief Return the channel for @p channelId, creating it on first use.
  ///
  /// Thread-safe: serialized against concurrent channel() and write() calls on this Writer. The
  /// returned reference stays valid for the Writer's lifetime. The Channel itself is not
  /// synchronized, so do not write to a single channel from multiple threads at once.
  [[nodiscard]] Channel& channel(ChannelId channelId);

  /// @brief Append @p data as one record on @p channelId and return a handle to the stored bytes.
  ///
  /// Convenience wrapper over channel(channelId).write(data). The returned Crate keeps its backing
  /// roll mapped for as long as it lives.
  ///
  /// Thread-safe with respect to other channels, but two threads must not write the same channel
  /// concurrently (see channel()).
  ///
  /// @throws std::runtime_error if the timeline is full.
  Crate write(ChannelId channelId, std::span<const std::byte> data);

  /// This chronicle's root directory.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  /// Maps each channel's id to the heap-owned Channel object, so a reference handed out by
  /// channel() stays valid even as the map rehashes.
  using ChannelMap = std::unordered_map<ChannelId, std::unique_ptr<Channel>>;

  /// The chronicle's root directory, returned by path().
  const std::filesystem::path mLogRoot;

  /// Byte size used when opening each channel's data roll.
  const std::size_t mRollCapacity;

  /// The shared timeline: one growable, memory-mapped array of TimelineEntry that records every
  /// channel's writes in global write order.
  containers::Tape<containers::MmapArray<TimelineEntry>> mTimeline;

  /// The channel registry guarded by a mutex, so concurrent channel() and write() calls serialize
  /// on it. The guarded ChannelMap owns every Channel for the Writer's lifetime.
  common::Locked<ChannelMap> mLockedChannelMap;
};

} // namespace nioc::chronicle
