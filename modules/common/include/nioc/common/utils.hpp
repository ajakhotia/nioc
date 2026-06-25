////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace nioc::common
{

/// @brief Returns the running program's name: the filename part of `argV[0]`, with any leading
/// directory path stripped.
///
/// Example:
///
///     int main(int argC, char** argV)
///     {
///       const std::string name = nioc::common::programName(argC, argV); // e.g. "myApp"
///     }
///
/// @param argC Argument count, passed straight through from `main`. When `<= 0`, the result is an
/// empty string and @p argV is never dereferenced.
///
/// @param argV Argument vector, passed straight through from `main`. May be `nullptr` only when
/// @p argC `<= 0`.
///
/// @return The basename of `argV[0]`, or an empty string when @p argC `<= 0`.
std::string programName(int argC, const char* const* argV);

} // namespace nioc::common
