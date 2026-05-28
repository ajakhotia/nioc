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

/// @brief Returns the top-level options group for a program, with Port's own section inside it.
///
/// The returned group is captioned with @p programName. It declares `--help` and contains a
/// separate "Port options" section. Add your application's own options to it, then pass it to
/// @ref parseCommandLine:
///
/// @code
/// auto options = nioc::terminus::programOptions("myProgramName");
///
/// // clang-format off
/// options.add_options()
/// (
///   "input,i",
///   boost::program_options::value<std::string>()->required(),
///   "Path to the input file"
/// )
/// (
///   "output,o",
///   boost::program_options::value<std::string>()->default_value("out.bin"),
///   "Path to the output file"
/// );
/// // clang-format on
///
/// const auto variableMap = nioc::terminus::parseCommandLine(argC, argV, options);
/// auto port = nioc::terminus::Port{ variableMap };
/// const auto& input = variableMap.at("input").as<std::string>();
/// const auto& output = variableMap.at("output").as<std::string>();
///
/// @endcode
///
/// Port's section declares: `--log-root`, `--append-config`, `--append-resource`,
/// `--record-chronicle`.
///
/// @param programName Name of the program.
[[nodiscard]] boost::program_options::options_description programOptions(
    const std::string& programName);

/// @brief Parses a command line against @p options, handling `--help` and errors by exiting.
///
/// Removes the order-sensitive parse boilerplate from every `main`. The steps:
///
/// - Stores the parse of @p argC / @p argV.
/// - If `--help` is present, prints @p options to `stdout` and exits with `EXIT_SUCCESS`.
/// - On a parse error, prints the error and @p options to `stderr` and exits with `EXIT_FAILURE`.
/// - Otherwise runs `notify`, then adds the verbatim launch command to the map under the key
///   `"commandLine"` (a non-option key, so `notify` leaves it untouched), and returns the map.
///
/// The help and error paths call `std::exit`, so this does not return in those cases.
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
