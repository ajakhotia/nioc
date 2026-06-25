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

static constexpr auto kRollFileNamePrefix = "roll";

static constexpr auto kFileNameExtension = ".nioc";

static constexpr auto kPaddedNumberLength = 20UL;

static constexpr auto kTimelineFileName = "timeline.nioc";

static constexpr auto kHexPrefix = "0x";

constexpr std::uint64_t roundUpToWord(const std::uint64_t value) noexcept
{
  constexpr auto kWord = std::uint64_t{8ULL};
  return (value + kWord - 1ULL) & ~(kWord - 1ULL);
}

std::string padString(const std::string& input, std::uint64_t paddedLength, char paddingChar = '0');

std::string buildRollName(std::uint64_t rollId);

template<typename Integer>
std::string hexString(const Integer integer)
{
  return std::format("{}{:x}", kHexPrefix, integer);
}

} // namespace nioc::chronicle
