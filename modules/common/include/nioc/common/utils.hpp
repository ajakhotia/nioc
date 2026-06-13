////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace nioc::common
{

/// @brief Returns the program name: @p argV[0] with leading directories stripped.
///
/// Use it in a usage or help header.
///
/// @param argC Argument count, as received by main.
///
/// @param argV Argument vector, as received by main.
///
/// @return The basename of @p argV[0]. Empty string when @p argC is 0.
std::string programName(int argC, const char* const* argV);

} // namespace nioc::common
