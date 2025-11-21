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

/// @brief Unique identifier for a data channel.
struct ChannelId
{
  std::uint64_t mValue;

  constexpr bool operator==(const ChannelId&) const = default;
};

/// @brief I/O mechanism for reading or writing chronicle data.
///
/// Specifies which implementation to use for IO access.
enum class IoMechanism : std::uint8_t
{
  Stream,

  Mmap
};

/// @brief Converts IoMechanism to string.
/// @param mechanism I/O mechanism to convert.
/// @return String representation of the mechanism.
/// @throws std::logic_error If the mechanism is unknown.
std::string stringFromIoMechanism(IoMechanism mechanism);

/// @brief Converts string to IoMechanism.
/// @param str String representation of the I/O mechanism.
/// @return IoMechanism enum value.
/// @throws std::out_of_range If the string is not a valid mechanism name.
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
