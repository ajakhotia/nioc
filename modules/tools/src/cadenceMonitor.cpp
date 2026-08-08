////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : ${PROJECT_NAME}
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cadenceMonitor.hpp"
#include <algorithm>
#include <capnp/any.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <nioc/common/time.hpp>
#include <nioc/concurrent/anyMpsc.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/message.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/schemaRegistry.hpp>
#include <nioc/terminus/tally.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <nioc/terminus/utils.hpp>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nioc::tools
{
namespace
{

/// The inbox is unbounded, so this is the initial reservation rather than a ceiling.
constexpr std::size_t kInboxCapacity = 1'024;

/// The route every payload is expected to carry: a `nioc.sensors` Header at `header` recording
/// the instant the tally observes.
constexpr std::string_view kTimestampFieldPath = "header.timestamp.nanosecondSinceEpoch";

/// @brief Reflect every schema the replayed topics carry into its timestamp extractor, keyed by
/// schema id. A schema the recording does not ship, or that does not carry the header timestamp
/// path, is warned about once, however many topics share it, and yields no entry.
///
/// @param componentName The monitor's name, prefixed onto the log lines.
///
/// @param topics The topics the recording replays; each names the schema its payloads carry.
///
/// @param schemas The recording's schemas, resolved through by schema id.
std::unordered_map<
    std::uint64_t,
    std::function<common::NamedClock::time_point(capnp::AnyPointer::Reader)>>
reflectTimestampExtractors(
    const std::string_view componentName,
    const terminus::TopicRegistry& topics,
    const terminus::SchemaRegistry& schemas)
{
  // Each referenced schema once, carrying its human-readable name for the log lines.
  auto schemaNameMap = std::unordered_map<std::uint64_t, std::string_view>{};
  for(const auto& topic: topics)
  {
    schemaNameMap.try_emplace(topic.mSchemaId, topic.mSchemaName);
  }

  auto extractorMap = std::unordered_map<
      std::uint64_t,
      std::function<common::NamedClock::time_point(capnp::AnyPointer::Reader)>>{};
  for(const auto& [schemaId, schemaName]: schemaNameMap)
  {
    if(not schemas.contains(schemaId))
    {
      logger::warn(
          "[{}] The recording ships no schema '{}'; its topics are listed but not measured.",
          componentName,
          schemaName);
      continue;
    }

    auto extractNanoseconds = terminus::dynamicFieldExtractor<std::int64_t>(
        schemas.at(schemaId),
        kTimestampFieldPath);
    if(not extractNanoseconds.has_value())
    {
      logger::warn(
          "[{}] Schema '{}' does not carry '{}'; its topics are listed but not measured.",
          componentName,
          schemaName,
          kTimestampFieldPath);
      continue;
    }

    // Bind the raw nanosecond read to the clock frame here, so subscribers receive the instant
    // ready to tally.
    extractorMap.emplace(
        schemaId,
        [extract = std::move(*extractNanoseconds)](const capnp::AnyPointer::Reader reader)
        { return common::NamedClock::time_point{common::NamedClock::duration{extract(reader)}}; });
  }

  return extractorMap;
}

/// Blank columns kept between one table column and the next, so the widest entry never abuts it.
constexpr auto kColumnGap = std::size_t{2};

/// @brief Print @p rows (the first being the heading row) as a left-aligned table on @p stream.
///
/// Each column is sized to the widest cell in it. The tally table joins a topic's identity columns
/// to its statistics columns, so this is where those two halves are laid out together.
void printTable(std::ostream& stream, const std::vector<std::vector<std::string>>& rows)
{
  auto widths = std::vector<std::size_t>{};
  for(const auto& row: rows)
  {
    widths.resize(std::max(widths.size(), row.size()), std::size_t{0});
    for(auto column = std::size_t{0}; column < row.size(); ++column)
    {
      widths.at(column) = std::max(widths.at(column), row.at(column).size());
    }
  }

  for(const auto& row: rows)
  {
    for(auto column = std::size_t{0}; column < row.size(); ++column)
    {
      stream << std::format("{:<{}}", row.at(column), widths.at(column) + kColumnGap);
    }
    stream << '\n';
  }
}

/// @brief Print the per-topic tally as a table: each topic's identity from @p registry, followed by
/// its statistics from @p tally, ordered by topic name; a topic with no messages shows as zeros.
///
/// The registry drives the rows and the tally supplies the numbers, joined by channel, so identity
/// is read from the registry only where a row needs it.
void printTally(
    std::ostream& stream,
    const terminus::TopicRegistry& registry,
    const terminus::Tally& tally)
{
  auto topics = std::vector<const terminus::Topic*>{};
  topics.reserve(registry.size());
  for(const auto& topic: registry)
  {
    topics.push_back(&topic);
  }
  std::ranges::sort(topics, {}, [](const terminus::Topic* topic) { return topic->mName; });

  auto rows = std::vector<std::vector<std::string>>{};

  auto heading = terminus::Topic::headings();
  const auto statHeading = terminus::ChannelStatistics::headings();
  heading.insert(heading.end(), statHeading.begin(), statHeading.end());
  rows.push_back(std::move(heading));

  for(const auto* const topic: topics)
  {
    auto row = topic->columns();
    const auto found = tally.find(topic->mChannelId);
    const auto statistics = (found != tally.end()) ? found->second : terminus::ChannelStatistics{};
    const auto columns = statistics.columns();
    row.insert(row.end(), columns.begin(), columns.end());
    rows.push_back(std::move(row));
  }

  printTable(stream, rows);
}

} // namespace

CadenceMonitor::CadenceMonitor(std::string name, terminus::Port& port):
  Component{std::move(name), port, kInboxCapacity, concurrent::BufferMode::Unbounded},
  mTopics{port.playbackTopics()}
{
  const auto extractorMap =
      reflectTimestampExtractors(this->name(), mTopics, port.playbackSchemas());

  // Subscribe to every replayed topic whose recorded schema carries the header convention. A topic
  // left unsubscribed simply never observes into the tally, so a report driven off the recording's
  // topics still shows it, at zero.
  auto subscribedCount = std::size_t{0};
  for(const auto& topic: mTopics)
  {
    if(not extractorMap.contains(topic.mSchemaId))
    {
      continue;
    }

    subscribe<capnp::AnyPointer>(
        topic.mChannelId,
        [this, channel = topic.mChannelId, extractTimestamp = extractorMap.at(topic.mSchemaId)](
            const terminus::Message<capnp::AnyPointer>& message)
        {
          return message.isGap() ? countGap()
                                 : this->tally(channel, extractTimestamp(message.reader()));
        });
    ++subscribedCount;
  }

  logger::info(
      "[{}] subscribed to {} of the {} replayed topics.",
      this->name(),
      subscribedCount,
      mTopics.size());
}

CadenceMonitor::~CadenceMonitor() noexcept
{
  const auto elapsed = std::chrono::duration<double>{std::chrono::steady_clock::now() - mStart};

  // Rendering is best-effort: the destructor must not throw, and a report that cannot be printed
  // should not take the teardown down with it.
  try
  {
    // The tally holds the numbers; the recording's topics supply each row's identity.
    printTally(std::cout, mTopics, mTally);

    std::cout << std::format(
        "\n{} messages in {:.3f} s ({:.0f} messages/s).\n",
        mTally.totalSamples(),
        elapsed.count(),
        static_cast<double>(mTally.totalSamples()) / elapsed.count());
  }
  catch(const std::exception& error)
  {
    logger::error("[{}] Failed to render the cadence report: {}", name(), error.what());
  }

  if(mGapCount > 0ULL)
  {
    logger::warn(
        "[{}] {} delivered entries carried no payload, marking gaps the tally does not count.",
        name(),
        mGapCount);
  }
}

terminus::Component::State CadenceMonitor::countGap() noexcept
{
  ++mGapCount;
  return State::Continue;
}

terminus::Component::State CadenceMonitor::tally(
    const ChannelId channelId,
    const common::NamedClock::time_point timePoint)
{
  mTally.observe(channelId, timePoint);
  return State::Continue;
}

} // namespace nioc::tools
