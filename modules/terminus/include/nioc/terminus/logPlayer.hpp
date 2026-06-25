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

/// @brief A source Driver that replays a recorded chronicle log onto a Port in recorded order.
///
/// Each tick delivers the next log entry to the Port on the channel it was captured on. It
/// reproduces order only, not the original inter-entry timing: one entry per tick, as fast as the
/// run ticks it. Wire it into the Driver/Routine lifecycle and let the Runner tick it; it finishes
/// at end of log or when shutdown is requested.
///
/// Example:
///
///     LogPlayer player{port, "/path/to/chronicleLog"};
///     // The Runner now ticks `player` until it reports State::Done.
///
/// Single use: the underlying reader cannot rewind. Construct a fresh instance to replay again.
///
/// @see Driver, chronicle::Reader
class LogPlayer final: public Driver
{
public:
  /// @brief Open a chronicle log for replay, seating the cursor on its first entry without
  /// delivering anything yet.
  ///
  /// @param port Borrowed; must outlive this player.
  ///
  /// @param inputLog Path to an existing chronicle log directory. An empty log is valid and the
  /// player completes on its first tick.
  ///
  /// @throws std::invalid_argument If @p inputLog does not name an existing directory.
  LogPlayer(Port& port, std::filesystem::path inputLog);

private:
  /// The opened chronicle log being replayed. Declared before mCursor because the cursor reads the
  /// first entry from this reader during its own construction, so the reader must already exist.
  chronicle::Reader mReader;

  /// The position of the next log entry to deliver. Advances by one on every tick and reaches the
  /// reader's end iterator once the whole log has been replayed.
  chronicle::Reader::Iterator mCursor;

  /// @brief Deliver the entry at mCursor to the Port, advance the cursor, and report whether replay
  /// should continue.
  ///
  /// @return State::Done once the cursor reaches the end of the log, otherwise the running state
  /// that asks the Runner to tick again.
  [[nodiscard]] State run() final;
};

} // namespace nioc::terminus
