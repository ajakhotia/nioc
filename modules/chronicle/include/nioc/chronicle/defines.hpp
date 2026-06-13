////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <string>

namespace nioc::chronicle
{

/// @brief Identifies one data channel.
struct ChannelId
{
  /// @brief The 64-bit id.
  std::uint64_t mValue;

  constexpr bool operator==(const ChannelId&) const = default;
};

/// @brief How chronicle data is read or written.
enum class IoMechanism : std::uint8_t
{
  /// @brief File streams. Use for writing.
  Stream,

  /// @brief Memory-mapped files. Use for reading.
  Mmap
};

/// @brief Returns the name of @p mechanism.
/// @throws std::logic_error If the mechanism is unknown.
std::string stringFromIoMechanism(IoMechanism mechanism);

/// @brief Returns the IoMechanism with the given name.
/// @param str A mechanism name (e.g. "Stream" or "Mmap").
/// @throws std::out_of_range If @p str is not a valid name.
IoMechanism ioMechanismFromString(const std::string& str);

} // namespace nioc::chronicle

template<>
struct std::hash<nioc::chronicle::ChannelId>
{
  decltype(auto) operator()(const nioc::chronicle::ChannelId& channelId) const noexcept
  {
    return std::hash<std::uint64_t>{}(channelId.mValue);
  }
};
