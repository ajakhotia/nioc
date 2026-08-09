////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <cstdint>
#include <nioc/common/time.hpp>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/tally.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <string>

namespace nioc::tools
{

/// @brief A Component that subscribes to every topic the replayed recording carries and measures
/// each channel's cadence, folding every delivered message into a Tally keyed on the timestamp in
/// the message header.
///
/// It reads the topology from the Port's @ref nioc::terminus::Port::playbackTopics registry and the
/// payload schemas from its @ref nioc::terminus::Port::playbackSchemas registry, so it knows what a
/// recording holds, and how to read it, without compiling against any payload type: each message is
/// read dynamically through the recording's own schema. The one convention it expects is that a
/// payload's `header` field is a `nioc.sensors` Header carrying `timestamp.nanosecondSinceEpoch`.
/// A topic whose schema was not recorded, or does not follow that convention, is warned about once
/// per schema and still given a tally series, so it shows in the report with a zero count rather
/// than vanishing; it is simply left unsubscribed.
///
/// Wire it into a run alongside a source that replays the recording and let the Runner tick it;
/// the monitor owns its tally and prints the per-topic report to standard out as it is destroyed.
/// The Port drains the inbox during teardown and the monitor outlives the Port, so by the time the
/// destructor renders the report the numbers are complete.
///
/// The inbox is unbounded on purpose. An overwriting inbox drops messages under load, and a dropped
/// message is one the tally silently never counts; the cost is that a consumer slower than its
/// source buffers the whole backlog.
///
/// @see nioc::terminus::Component, nioc::terminus::Tally, nioc::terminus::SchemaRegistry
class CadenceMonitor final: public terminus::Component
{
public:
  /// @brief Subscribe to every replayed topic whose recorded schema carries the header
  /// convention, and start the observation clock the report's throughput line is measured against.
  ///
  /// @param name Identifying label, fixed for the monitor's lifetime.
  ///
  /// @param port The Port delivering the messages; its playback registries supply the topology and
  /// the schemas. Must outlive this monitor.
  CadenceMonitor(std::string name, terminus::Port& port);

  CadenceMonitor(const CadenceMonitor&) = delete;

  CadenceMonitor(CadenceMonitor&&) noexcept = delete;

  /// @brief Print the per-topic report and the throughput line to standard out, warn about any
  /// gaps seen, then release.
  ///
  /// The report is rendered here rather than by the hosting main because the run's teardown
  /// destroys the monitor last, when the tally is finally complete; a main sees only a dead run.
  ~CadenceMonitor() noexcept final;

  CadenceMonitor& operator=(const CadenceMonitor&) = delete;

  CadenceMonitor& operator=(CadenceMonitor&&) noexcept = delete;

private:
  /// The topics the recording replays, copied at construction so the report can still name every
  /// row at teardown, after the Port is gone.
  terminus::TopicRegistry mTopics;

  /// The tally every subscription feeds; complete once the Port has drained during teardown.
  terminus::Tally mTally;

  /// When observation began, anchoring the throughput line of the report.
  std::chrono::steady_clock::time_point mStart{std::chrono::steady_clock::now()};

  /// Count of delivered entries that carried no payload, marking gaps in the recording.
  std::uint64_t mGapCount{0ULL};

  /// @brief Count one gap delivery: an entry that carried no payload observes nothing, and the
  /// count is reported at teardown.
  [[nodiscard]] State countGap() noexcept;

  /// @brief Fold one instant into the tally, keyed on the channel that delivered it.
  ///
  /// @param channelId The channel the message was delivered on, keying the tally series.
  ///
  /// @param timePoint The instant the message's header records.
  [[nodiscard]] State tally(ChannelId channelId, common::NamedClock::time_point timePoint);
};

} // namespace nioc::tools
