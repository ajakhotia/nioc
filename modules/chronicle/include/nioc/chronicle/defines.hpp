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

/// @brief Identifies one data channel.
struct ChannelId
{
  /// @brief The 64-bit id.
  std::uint64_t mValue{};

  constexpr bool operator==(const ChannelId&) const = default;
};

/// @brief One ordering record: locates a recorded frame and names its channel.
///
/// The timeline is an array of these in record order, spread across a growing run of files, so a
/// reader replays every channel in one order. Stored as its raw bytes, in host byte order.
struct TimelineEntry
{
  ChannelId mChannelId;

  std::uint64_t mRollId{0ULL};

  std::uint64_t mOffset{0ULL};

  std::uint64_t mSize{0ULL};
};

/// @brief Combines a payload type id and a topic name into a channel id.
///
/// Equal inputs always give the same channel; the same type on two topics maps to two channels.
///
/// @param typeId Identifier of the payload type carried on the channel.
///
/// @param topic Topic name.
///
/// @return The channel for this type-and-topic pair.
ChannelId makeChannelId(std::uint64_t typeId, std::string_view topic);

} // namespace nioc::chronicle

template<>
struct std::hash<nioc::chronicle::ChannelId>
{
  decltype(auto) operator()(const nioc::chronicle::ChannelId& channelId) const noexcept
  {
    return std::hash<std::uint64_t>{}(channelId.mValue);
  }
};
