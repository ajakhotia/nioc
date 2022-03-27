////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <fstream>
#include <span>
#include <spdlog/fmt/fmt.h>
#include <vector>

namespace naksh::logger
{

/// Prefix of a data roll within a channel.
static constexpr auto kRollFileNamePrefix = "roll";

/// File extension of a data roll within a channel.
static constexpr auto kRollFileNameExtension = ".nio";

/// Name of the index file within a channel.
static constexpr auto kIndexFileName = "index";


/// @brief  Converts a system_clock time_point to date-time string formatted per ISO 8601
/// @param  timePoint   Time point to be converted.
/// @return Date-time string.
std::string timeAsFormattedString(std::chrono::system_clock::time_point timePoint);


/// @brief  Pads the input string such that the output string is paddedLength long. If the size of
///         the input is already greater than or equal to the paddedLength, then no padding is
///         performed.
/// @param  input           Input string.
/// @param  paddedLength    Length of the output string.
/// @param  paddingChar     The character used to pad the input. [Default: '0' (Zero)]
/// @return The padded string.
std::string padString(const std::string& input, uint64_t paddedLength, char paddingChar = '0');


/// @brief  Converts an integer to a sting in hexadecimal form(0x.....).
/// @tparam Integer The integer type.
/// @param  integer Input.
/// @return A string containing the integer represented in hexadecimal form.
template<typename Integer>
std::string toHexString(const Integer integer)
{
    return fmt::format("0x{:x}", integer);
}


/// @brief  Converts a vaild hex string to an integer. The string must start with 0x.
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
        throw std::invalid_argument("[Logger::hexStringToInteger] Provided input does not start " +
                                    std::string(kHexPrefix) + " prefix.");
    }

    return std::stoull(hexString, nullptr, kHexBase);
}


/// @brief  Checks if the files has required amount of space before reaching the max size.
/// @param  file                Reference to the file in question.
/// @param  spaceRequired       Space required by the client.
/// @param  maxFileSizeInBytes  Maximum allowable size of the file.
/// @return True if the required space is available. False otherwise.
bool fileHasSpace(std::ofstream& file, size_t spaceRequired, size_t maxFileSizeInBytes);


/// @brief  Compute the sum of the length of each byte span in the collection.
/// @param  dataCollection A collection of ConstByteSpan.
/// @return Total size in bytes.
size_t computeTotalSizeInBytes(const std::vector<std::span<const std::byte>>& dataCollection);


/// @brief  Writes a uint64_t to a file in little-endian format.
/// @param  file File to write to.
/// @param  integer Value to be written.
void writeToFile(std::ofstream& file, uint64_t integer);


/// @brief  Write a span of bytes to a file.
/// @param  file    File to write to.
/// @param  data    Span of bytes.
void writeToFile(std::ofstream& file, const std::span<const std::byte>& data);


} // namespace naksh::logger
