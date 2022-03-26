////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <fstream>
#include <span>
#include <vector>

namespace naksh::logger
{

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
