////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>
#include <string>

namespace nioc::terminus
{

/// @brief Returns the top-level options group, captioned @p programName and declaring `--help`.
///
/// Add subsystem and application flags to the group, then pass it to @ref parseCommandLine.
///
/// @code
/// auto options = nioc::terminus::programOptions("myProgramName");
/// options.add(nioc::terminus::Manifest::cliOptions());
///
/// // clang-format off
/// options.add_options()
/// (
///   "input,i",
///   boost::program_options::value<std::string>()->required(),
///   "Path to the input file"
/// );
/// // clang-format on
///
/// const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);
/// const auto& input = variableMap.at("input").as<std::string>();
/// @endcode
///
/// @param programName Program name.
[[nodiscard]] boost::program_options::options_description programOptions(
    const std::string& programName);

/// @brief Parses the command line against @p options and returns the variables map.
///
/// With `--help`, prints @p options to `stdout` and exits with `EXIT_SUCCESS`. On a parse error,
/// prints the error and @p options to `stderr` and exits with `EXIT_FAILURE`. Neither case returns.
///
/// On success, the map also holds a `"commandLine"` string entry with the verbatim launch command.
///
/// @param argC Number of entries in @p argV (usually `argc` from `main`).
///
/// @param argV Launch arguments (usually `argv` from `main`).
///
/// @param options Options group to parse against (see @ref programOptions).
///
/// @return The parsed options, plus a `"commandLine"` string entry.
[[nodiscard]] boost::program_options::variables_map parseCommandLine(
    int argC,
    const char* const* argV,
    const boost::program_options::options_description& options);

} // namespace nioc::terminus
