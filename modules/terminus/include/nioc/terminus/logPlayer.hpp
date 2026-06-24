////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "driver.hpp"
#include <filesystem>
#include <nioc/chronicle/reader.hpp>
#include <string>

namespace nioc::terminus
{

/// @brief A @ref Driver that replays a recorded chronicle onto a Port in record order.
///
/// Reads the chronicle at the input log and, one frame per @ref run, delivers it to the subscribers
/// of its channel - so the run sees the recorded frames in the exact global order they were first
/// recorded, across every channel. Replay is schema-agnostic: each frame is delivered as its raw
/// recorded bytes, and the receiving subscriber decodes it as its own @ref Message.
///
/// Replay delivers; it does not re-record. The frames are not written into the current run's
/// chronicle. Pacing is not modelled: frames are delivered as fast as the run consumes them, not at
/// their original arrival cadence.
class LogPlayer final: public Driver
{
public:
  /// @brief Replays the chronicle in @p inputLog onto @p port.
  ///
  /// @param port Port whose subscribers receive the replayed frames; must outlive this player.
  ///
  /// @param inputLog Chronicle directory to replay.
  ///
  /// @throws std::invalid_argument If @p inputLog does not exist or is not a directory.
  LogPlayer(Port& port, std::filesystem::path inputLog);

private:
  // mReader is declared before mCursor: the cursor reads the first frame from the reader as it is
  // constructed, so the reader must already exist.
  chronicle::Reader mReader;
  chronicle::Reader::Iterator mCursor;

  /// @brief Delivers the frame under the cursor, then advances it.
  ///
  /// @return @ref State::Continue while frames remain, @ref State::Done once the log is exhausted
  /// or a shutdown has been requested.
  [[nodiscard]] State run() final;
};

} // namespace nioc::terminus
