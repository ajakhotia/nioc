////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <filesystem>
#include <nioc/chronicle/defines.hpp>
#include <span>

namespace nioc::chronicle
{

/// Prefix of a data roll within a channel.
static constexpr auto kRollFileNamePrefix = "roll";

/// File extension of a data roll within a channel.
static constexpr auto kRollFileNameExtension = ".nioc";

/// Length of the padded roll number string.
static constexpr auto kPaddedRollNumberLength = 20UL;

/// Name of the index file within a channel.
static constexpr auto kIndexFileName = "index";

/// Name of the sequence file in a log.
static constexpr auto kSequenceFileName = "sequence";

/// @brief  A structure that represents an entry in the sequence file of a log.
struct SequenceEntry
{
  ChannelId mChannelId;
};

/// @brief  A structure that represents an entry in the index file of a channel.
struct IndexEntry
{
  std::uint64_t mRollId;

  std::uint64_t mRollPosition;

  std::uint64_t mDataSize;
};

/// @brief  Converts a system_clock time_point to date-time string formatted per ISO 8601
/// @param  timePoint   Time point to be converted.
/// @return Date-time string.
std::string iso8601UtcFormat(std::chrono::system_clock::time_point timePoint);


/// @brief  Pads the input string such that the output string is paddedLength long. If the size of
///         the input is already greater than or equal to the paddedLength, then no padding is
///         performed.
/// @param  input           Input string.
/// @param  paddedLength    Length of the output string.
/// @param  paddingChar     The character used to pad the input. [Default: '0' (Zero)]
/// @return The padded string.
std::string padString(const std::string& input, uint64_t paddedLength, char paddingChar = '0');


/// @brief  Builds the log roll file name from rollId.
/// @param  rollId  Integer identifying the roll.
/// @return std::string containing the name for the roll.
std::string buildRollName(std::uint64_t rollId);

/// @brief  Converts an integer to a sting in hexadecimal form (0x...)
/// @tparam Integer The integer type.
/// @param  integer Input.
/// @return A string containing the integer represented in hexadecimal form.
template<typename Integer>
std::string toHexString(const Integer integer)
{
  return std::format("0x{:x}", integer);
}

/// @brief  Converts a valid hex string to an integer. The string must start with 0x.
/// @tparam Integer     Integer type to return.
/// @param  hexString   Input hex string
/// @return Equivalent integer.
template<typename Integer>
Integer hexStringToInteger(const std::string& hexString)
{
  static constexpr auto kHexPrefix = "0x";
  static constexpr auto kHexBase = 16U;
  if(not hexString.starts_with(kHexPrefix))
  {
    throw std::invalid_argument(
        "[Logger::hexStringToInteger] Provided input does not start " + std::string(kHexPrefix) +
        " prefix.");
  }

  return std::stoull(hexString, nullptr, kHexBase);
}

/// @brief  Checks if the files has required amount of space before reaching the max size.
/// @param  file                Reference to the file in question.
/// @param  spaceRequired       Space required by the client.
/// @param  maxFileSizeInBytes  Maximum allowable size of the file.
/// @return True if the required space is available. False otherwise.
bool
fileHasSpace(std::ofstream& file, std::uint64_t spaceRequired, std::uint64_t maxFileSizeInBytes);


/// @brief  Compute the sum of the length of each byte span in the collection.
/// @param  dataCollection A collection of ConstByteSpan.
/// @return Total size in bytes.
std::uint64_t computeTotalSizeInBytes(std::span<const std::span<const std::byte>> dataCollection);

/// @brief  Struct used to provide read/write functionality for a give type.
/// @tparam ValueType
template<typename ValueType>
struct ReadWriteUtil
{
  static void write(std::ostream& stream, ValueType value);

  static ValueType read(const char* ptr, std::uint64_t size = sizeof(ValueType));
};

/// @brief  Ensure that the input path exists and returns the same.
std::filesystem::path validatePath(std::filesystem::path path);


} // namespace nioc::chronicle
