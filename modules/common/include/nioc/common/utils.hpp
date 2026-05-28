////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace nioc::common
{

/// @brief Returns the program's invocation name — the basename of @p argV[0].
///
/// This is how the program was launched, with any leading directory components removed. It is
/// the name conventionally shown in a usage or help header.
///
/// @param argC Argument count, exactly as received by main.
///
/// @param argV Argument vector, exactly as received by main.
///
/// @return The basename of @p argV[0], or an empty string when @p argC is 0 (no @p argV[0]).
std::string programName(int argC, const char* const* argV);

} // namespace nioc::common
