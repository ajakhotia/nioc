////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <nioc/chronicle/defines.hpp>
#include <nioc/common/exception.hpp>
#include <nioc/common/utils.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/topicRegistry.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

// The topics file: its name and JSON keys, the single definition both write() and the loading
// constructor read. Ids are stored as hex strings so a person can match them against the channel
// directories and the schema bank by eye, and so a 64-bit id survives the file exactly.
constexpr auto kFileName = "topics.json";
constexpr auto kTopicsKey = "topics";
constexpr auto kChannelIdKey = "channelId";
constexpr auto kNameKey = "topic";
constexpr auto kSchemaIdKey = "schemaId";
constexpr auto kSchemaNameKey = "schemaName";

// Blank columns kept between one table column and the next, so the widest entry never abuts it.
constexpr auto kColumnGap = std::size_t{2};

// The topics ordered by name, for a stable file and a stable table.
std::vector<const Topic*> orderedByName(const TopicRegistry& registry)
{
  auto ordered = std::vector<const Topic*>{};
  ordered.reserve(registry.size());
  for(const auto& topic: registry)
  {
    ordered.push_back(&topic);
  }
  std::ranges::sort(ordered, {}, [](const Topic* topic) { return topic->mName; });

  return ordered;
}

} // namespace

std::vector<std::string> Topic::headings()
{
  return {"topic", "schemaName", "schemaId", "channelId"};
}

std::vector<std::string> Topic::columns() const
{
  return {mName, mSchemaName, common::hexString(mSchemaId), common::hexString(mChannelId.mValue)};
}

TopicRegistry::TopicRegistry(const fs::path& recording)
{
  // A live run names no input log; there is nothing to adopt.
  if(recording.empty())
  {
    return;
  }

  const auto topicsPath = recording / kFileName;
  try
  {
    if(not fs::is_regular_file(topicsPath))
    {
      common::throwException<std::runtime_error>("no {}", kFileName);
    }

    for(const auto& entry: readJsonFile(topicsPath).at(kTopicsKey))
    {
      record(
          chronicle::ChannelId{
              common::fromHexString<std::uint64_t>(entry.at(kChannelIdKey).get<std::string>())},
          entry.at(kNameKey).get<std::string>(),
          common::fromHexString<std::uint64_t>(entry.at(kSchemaIdKey).get<std::string>()),
          entry.at(kSchemaNameKey).get<std::string>());
    }
  }
  catch(const std::exception& error)
  {
    // A record that cannot be read costs introspection, not the replay: the timeline still delivers
    // by channel. Report it and carry on with whatever was read.
    logger::error("Could not adopt topics from {}: {}", recording.string(), error.what());
  }
}

void TopicRegistry::record(
    const chronicle::ChannelId channelId,
    std::string name,
    const std::uint64_t schemaId,
    std::string schemaName)
{
  // A channel takes one publisher, so a channel already present is refused whatever its topic: a
  // repeat is a second publisher, and a differing topic additionally signals a makeChannelId
  // collision or inconsistent metadata.
  if(const auto entry = std::ranges::find(*this, channelId, &Topic::mChannelId); entry != end())
  {
    common::throwException<std::runtime_error>(
        "Channel {} already carries topic '{}'; a channel takes one publisher.",
        common::hexString(channelId.mValue),
        entry->mName);
  }

  insert(
      Topic{
          .mChannelId = channelId,
          .mName = std::move(name),
          .mSchemaId = schemaId,
          .mSchemaName = std::move(schemaName)});
}

void TopicRegistry::write(const fs::path& directory) const
{
  auto entries = nlohmann::json::array();
  for(const auto* const topic: orderedByName(*this))
  {
    entries.push_back(
        nlohmann::json{
            {kChannelIdKey, common::hexString(topic->mChannelId.mValue)},
            {kNameKey, topic->mName},
            {kSchemaIdKey, common::hexString(topic->mSchemaId)},
            {kSchemaNameKey, topic->mSchemaName}});
  }

  writeJsonFile(directory / kFileName, nlohmann::json{{kTopicsKey, entries}});
}

std::ostream& operator<<(std::ostream& stream, const TopicRegistry& registry)
{
  const auto headings = Topic::headings();
  const auto ordered = orderedByName(registry);

  // Each column is as wide as the widest of its heading and every value beneath it.
  auto widths = std::vector<std::size_t>{};
  widths.reserve(headings.size());
  for(const auto& heading: headings)
  {
    widths.push_back(heading.size());
  }
  for(const auto* const topic: ordered)
  {
    const auto columns = topic->columns();
    for(auto column = std::size_t{0}; column < widths.size(); ++column)
    {
      widths.at(column) = std::max(widths.at(column), columns.at(column).size());
    }
  }

  const auto emitRow = [&stream, &widths](const std::vector<std::string>& row)
  {
    for(auto column = std::size_t{0}; column < row.size(); ++column)
    {
      stream << std::format("{:<{}}", row.at(column), widths.at(column) + kColumnGap);
    }
    stream << '\n';
  };

  emitRow(headings);
  for(const auto* const topic: ordered)
  {
    emitRow(topic->columns());
  }

  return stream;
}

} // namespace nioc::terminus
