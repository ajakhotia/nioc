////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"
#include <algorithm>

namespace nioc::chronicle
{

std::string padString(
    const std::string& input,
    const std::uint64_t paddedLength,
    const char paddingChar)
{
  return std::string(paddedLength - std::min(paddedLength, input.size()), paddingChar) + input;
}

std::string buildRollName(const std::uint64_t rollId)
{
  return kRollFileNamePrefix + padString(std::to_string(rollId), kPaddedNumberLength) +
         kFileNameExtension;
}

} // namespace nioc::chronicle
