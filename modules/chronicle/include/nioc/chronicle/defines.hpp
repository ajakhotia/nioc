////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <string_view>

namespace nioc::chronicle
{

/// @brief A value type that names one logging channel and serves as its map key across the
/// chronicle.
///
/// Obtain ids from makeChannelId; do not set @ref mValue by hand. Two ids are equal exactly when
/// their values match.
///
/// @see makeChannelId, std::hash<nioc::chronicle::ChannelId>
struct ChannelId
{
  /// The 64-bit hash that identifies the channel.
  std::uint64_t mValue{};

  /// Compare two ids by value.
  constexpr bool operator==(const ChannelId&) const = default;
};

/// @brief A value type that pinpoints one logged record: its channel, the data file (roll) holding
/// the bytes, and the record's half-open byte range `[mOffset, mOffset + mSize)` within that roll.
struct TimelineEntry
{
  /// The channel that owns the record.
  ChannelId mChannelId;

  /// The roll (data file) within the channel that stores the record's bytes.
  std::uint64_t mRollId{0ULL};

  /// Byte offset of the record within the roll.
  std::uint64_t mOffset{0ULL};

  /// Length of the record in bytes.
  std::uint64_t mSize{0ULL};
};

/// @brief Compute the channel id for a topic of a given message type.
///
/// Example:
///
///     auto id = makeChannelId(messageTypeId, "/camera/image");
///
/// Equal `(typeId, topic)` pairs always produce equal ids; distinct pairs almost always produce
/// distinct ids, but collisions are possible because the id is a 64-bit hash.
///
/// @param typeId Identifies the message type carried on the topic.
///
/// @param topic Hashed by content; not retained after the call.
ChannelId makeChannelId(std::uint64_t typeId, std::string_view topic);

} // namespace nioc::chronicle

/// @brief std::hash specialization that lets ChannelId be used as a key in std::unordered_map,
/// std::unordered_set, and similar containers.
///
/// @see ChannelId
template<>
struct std::hash<nioc::chronicle::ChannelId>
{
  /// Hash an id by hashing its value.
  decltype(auto) operator()(const nioc::chronicle::ChannelId& channelId) const noexcept
  {
    return std::hash<std::uint64_t>{}(channelId.mValue);
  }
};
