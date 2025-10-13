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
using ChannelId = std::uint64_t;

/// @brief I/O mechanism for reading or writing chronicle data.
///
/// Specifies which implementation to use for data access.
/// Each mechanism corresponds to a specific ChannelReader/ChannelWriter implementation.
enum class IoMechanism : std::uint8_t
{
  /// File-based streaming I/O (sequential writes to rotating files).
  Stream,

  /// Memory-mapped file I/O (fast random access to existing data).
  Mmap
};

/// @brief Converts IoMechanism to string.
/// @param mechanism I/O mechanism to convert.
/// @return String representation of the mechanism.
/// @throws std::out_of_range If the mechanism is unknown.
std::string stringFromIoMechanism(IoMechanism mechanism);

/// @brief Converts string to IoMechanism.
/// @param str String representation of the I/O mechanism.
/// @return IoMechanism enum value.
/// @throws std::out_of_range If the string is not a valid mechanism name.
IoMechanism ioMechanismFromString(const std::string& str);

} // namespace nioc::chronicle
