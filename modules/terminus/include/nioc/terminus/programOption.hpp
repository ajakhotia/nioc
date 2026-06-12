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

/// @brief Returns the top-level options group for a program, declaring `--help`.
///
/// The returned group is captioned with @p programName. Each subsystem the program uses
/// contributes its own section via `add()` — see @ref Port::cliOptions and @ref
/// ConfigStore::cliOptions — and the application appends its own flags, then passes the group to
/// @ref parseCommandLine:
///
/// @code
/// auto options = nioc::terminus::programOptions("myProgramName");
/// options.add(nioc::terminus::Port::cliOptions());
/// options.add(nioc::terminus::ConfigStore::cliOptions());
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
/// @param programName Name of the program.
[[nodiscard]] boost::program_options::options_description programOptions(
    const std::string& programName);

/// @brief Parses the command line against @p options and returns the populated variables map.
///
/// Handles the two terminal cases so the caller never has to: with `--help`, prints @p options to
/// `stdout` and exits with `EXIT_SUCCESS`; on a parse error, prints the error and @p options to
/// `stderr` and exits with `EXIT_FAILURE`. Neither path returns.
///
/// On success, the returned map also carries a `"commandLine"` string entry holding the verbatim
/// launch command.
///
/// @param argC Number of entries in @p argV (typically `argc` from `main`).
///
/// @param argV Launch arguments (typically `argv` from `main`).
///
/// @param options The full options group to parse against (see @ref programOptions).
///
/// @return The parsed options, including a `"commandLine"` string entry.
[[nodiscard]] boost::program_options::variables_map parseCommandLine(
    int argC,
    const char* const* argV,
    const boost::program_options::options_description& options);

} // namespace nioc::terminus
