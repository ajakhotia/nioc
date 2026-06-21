////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <format>
#include <nioc/chronicle/defines.hpp>
#include <string>

namespace nioc::chronicle
{

/// Prefix of a data roll within a channel.
static constexpr auto kRollFileNamePrefix = "roll";

/// File extension of a chronicle file.
static constexpr auto kFileNameExtension = ".nioc";

/// Length of the padded file-number string.
static constexpr auto kPaddedNumberLength = 20UL;

/// Directory holding the chronicle's timeline files.
static constexpr auto kTimelineDirName = "timeline";

/// Prefix of a timeline file.
static constexpr auto kTimelineFileNamePrefix = "timeline";

/// Prefix marking a hexadecimal string representation.
static constexpr auto kHexPrefix = "0x";

/// @brief Rounds @p value up to the next multiple of the 8-byte word that frames are aligned to.
constexpr std::uint64_t roundUpToWord(const std::uint64_t value) noexcept
{
  constexpr auto kWord = std::uint64_t{8ULL};
  return (value + kWord - 1ULL) & ~(kWord - 1ULL);
}

/// @brief One ordering record: locates a recorded frame and names its channel.
///
/// The timeline is an array of these in record order, spread across files in @ref kTimelineDirName,
/// so a reader replays every channel in order. Stored as its raw bytes (host byte order).
struct TimelineEntry
{
  ChannelId mChannelId;

  std::uint64_t mRollId{0ULL};

  std::uint64_t mOffset{0ULL};

  std::uint64_t mSize{0ULL};
};

/// @brief Pads @p input on the left with @p paddingChar to @p paddedLength characters.
///
/// Returns @p input unchanged when it is already at least @p paddedLength long.
std::string padString(const std::string& input, std::uint64_t paddedLength, char paddingChar = '0');

/// @brief Builds the roll file name for @p rollId.
std::string buildRollName(std::uint64_t rollId);

/// @brief Builds the timeline file name for @p fileIndex.
std::string buildTimelineFileName(std::uint64_t fileIndex);

/// @brief Formats @p integer as a hexadecimal string (e.g. `0x1f`).
template<typename Integer>
std::string hexString(const Integer integer)
{
  return std::format("{}{:x}", kHexPrefix, integer);
}

} // namespace nioc::chronicle
