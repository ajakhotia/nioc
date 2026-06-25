////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

namespace nioc::terminus
{

/// @brief The 64-bit type id of a Cap'n Proto schema, as a compile-time constant.
///
/// The value is the schema's `@0x...` type id. It is the same on every run and every machine, and
/// two different schemas have two different values. Use it to tag a topic with its message type
/// when building a channel id, so a publisher and a subscriber that agree on the schema and topic
/// arrive at the same channel without hand-picking ids.
///
/// Example:
///
///     // MySchema is a generated Cap'n Proto struct type.
///     const auto channelId = chronicle::makeChannelId(kSchemaId<MySchema>, "imu");
///
/// @tparam Schema A generated Cap'n Proto struct type. Pass it explicitly; it is not deduced.
///
/// @see chronicle::makeChannelId
template<typename Schema>
inline constexpr std::uint64_t kSchemaId = static_cast<std::uint64_t>(
    Schema::_capnpPrivate::typeId);

} // namespace nioc::terminus
