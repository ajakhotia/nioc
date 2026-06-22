////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <format>
#include <string>

namespace nioc::chronicle
{

/// Prefix of a data roll within a channel.
static constexpr auto kRollFileNamePrefix = "roll";

/// File extension of a chronicle file.
static constexpr auto kFileNameExtension = ".nioc";

/// Length of the padded file-number string.
static constexpr auto kPaddedNumberLength = 20UL;

/// Name of the chronicle's single timeline file.
static constexpr auto kTimelineFileName = "timeline.nioc";

/// Prefix marking a hexadecimal string representation.
static constexpr auto kHexPrefix = "0x";

/// @brief Rounds @p value up to the next multiple of the 8-byte word that frames are aligned to.
constexpr std::uint64_t roundUpToWord(const std::uint64_t value) noexcept
{
  constexpr auto kWord = std::uint64_t{8ULL};
  return (value + kWord - 1ULL) & ~(kWord - 1ULL);
}

/// @brief Pads @p input on the left with @p paddingChar to @p paddedLength characters.
///
/// Returns @p input unchanged when it is already at least @p paddedLength long.
std::string padString(const std::string& input, std::uint64_t paddedLength, char paddingChar = '0');

/// @brief Builds the roll file name for @p rollId.
std::string buildRollName(std::uint64_t rollId);

/// @brief Formats @p integer as a hexadecimal string (e.g. `0x1f`).
template<typename Integer>
std::string hexString(const Integer integer)
{
  return std::format("{}{:x}", kHexPrefix, integer);
}

} // namespace nioc::chronicle
