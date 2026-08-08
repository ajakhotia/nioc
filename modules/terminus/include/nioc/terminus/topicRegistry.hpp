////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <nioc/chronicle/defines.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace nioc::terminus
{

/// @brief One topic a recording carries: the channel it is delivered on, the name it was published
/// under, and the identity of its payload schema.
///
/// The schema is named twice on purpose. `mSchemaId` is the schema's stable Cap'n Proto type id,
/// the thing to key on because it never changes. `mSchemaName` is the human-readable spelling, kept
/// for the file and for reports; treat it as a label, not an identifier.
struct Topic
{
  /// The channel the topic's messages are delivered oI n.
  chronicle::ChannelId mChannelId;

  /// The topic name the messages were published under.
  std::string mName;

  /// The stable Cap'n Proto type id of the payload schema.
  std::uint64_t mSchemaId{0ULL};

  /// The human-readable name of the payload schema.
  std::string mSchemaName;

  /// Compare by value.
  bool operator==(const Topic&) const = default;

  /// @brief The column headings a topic renders under, in order.
  ///
  /// One definition of how a topic is identified in text, drawn on both by the registry's table and
  /// by any tabular report built over topics.
  [[nodiscard]] static std::vector<std::string> headings();

  /// @brief This topic's values, in the same order as @ref headings; the ids as hex.
  [[nodiscard]] std::vector<std::string> columns() const;
};

} // namespace nioc::terminus

/// @brief Hash of a @ref Topic on its channel, the field that identifies it.
///
/// Consistent with `Topic::operator==`: equal topics share a channel, so they hash alike. Because
/// the hash keys on the channel alone while equality compares every field, two topics that collide
/// on a channel yet differ elsewhere land in one bucket without being treated as equal, so a
/// @ref nioc::chronicle::makeChannelId collision is caught rather than silently merged.
template<>
struct std::hash<nioc::terminus::Topic>
{
  [[nodiscard]] std::size_t operator()(const nioc::terminus::Topic& topic) const noexcept
  {
    return std::hash<nioc::chronicle::ChannelId>{}(topic.mChannelId);
  }
};

namespace nioc::terminus
{

/// @brief The topics a recording carries, and the sole owner of the `topics.json` convention.
///
/// Every part of the system that needs to know what a recording holds, whether a run building its
/// own record as publishers open, a replay adopting the record of the log it plays, or a tool
/// inspecting a log on disk, goes through this one type. The on-disk format lives here and nowhere
/// else, so it cannot drift and a reader can never fall out of step with a writer.
///
/// It is a `std::unordered_set<Topic>` that hashes on the channel (see the `std::hash<Topic>`
/// specialization) but compares on every field. @ref record layers the single-publisher-per-channel
/// rule on top: a channel already present is refused, so at most one topic per channel survives.
/// Iteration, `size`, `empty`, and `find` come straight from the set.
///
/// This type adds the recording, loading, and table rendering. Use @ref record rather than raw
/// `insert`: it builds the topic and reports a channel recorded twice as an error.
///
/// Example:
///
///     // Build a recording's record and persist it.
///     auto registry = TopicRegistry{};
///     registry.record(channelId, "/imu", schemaId, "nioc::sensors::Imu");
///     registry.write(recordingDir);
///
///     // Adopt an existing recording's record and print it.
///     std::cout << TopicRegistry{recordingDir};
class TopicRegistry: public std::unordered_set<Topic>
{
public:
  /// @brief An empty registry, as a live run starts with.
  TopicRegistry() = default;

  /// @brief Adopt the record of the recording at @p recording, if it has one.
  ///
  /// An empty @p recording (a live run naming no input log) yields an empty registry. A @p
  /// recording that carries no readable, well-formed `topics.json` also yields an empty registry,
  /// with the reason logged: a replay without topic names still delivers by channel, so this is a
  /// loss of introspection, not a failed run.
  ///
  /// @param recording Path to a recording directory, or an empty path.
  explicit TopicRegistry(const std::filesystem::path& recording);

  /// @brief Enter one topic, identified by its channel.
  ///
  /// A channel takes one topic. Recording a channel already present is an error, whether the topic
  /// matches or not: a repeat means a second publisher on the channel, and a mismatch additionally
  /// signals a @ref chronicle::makeChannelId collision or inconsistent metadata. Either way the
  /// by-channel storage cannot hold two, so the second record is refused.
  ///
  /// @param channelId The channel the topic is delivered on; identifies it.
  ///
  /// @param name The topic name.
  ///
  /// @param schemaId The payload schema's stable type id.
  ///
  /// @param schemaName The payload schema's human-readable name.
  ///
  /// @throws std::runtime_error If a topic is already recorded on @p channelId.
  void record(
      chronicle::ChannelId channelId,
      std::string name,
      std::uint64_t schemaId,
      std::string schemaName);

  /// @brief Write the record to `topics.json` under @p directory, ordered by topic name so the file
  /// is stable across runs.
  ///
  /// @param directory An existing directory to write into.
  ///
  /// @throws std::runtime_error If the file cannot be written.
  void write(const std::filesystem::path& directory) const;
};

/// @brief Emit @p registry as a table: a heading row, then one row per topic ordered by name.
std::ostream& operator<<(std::ostream& stream, const TopicRegistry& registry);

} // namespace nioc::terminus
