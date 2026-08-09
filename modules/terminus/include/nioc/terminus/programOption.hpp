////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/program_options.hpp>

namespace nioc::terminus
{

/// @brief Parse a process's argument vector against @p options and return the populated variable
/// map, handling `--help` and errors by terminating the process.
///
/// Call this once from `main`. The standard `--help`/`-h` flag is registered here on top of
/// @p options, so callers never declare it. On success the returned map also holds a
/// `"commandLine"` string entry: every argument joined by single spaces.
///
/// Example:
///
///   int main(int argC, char** argV)
///   {
///       auto options = RunContext::cliOptions();
///       options.add_options()("count,c", boost::program_options::value<int>(), "Item count");
///       const auto variableMap = parseCommandLine(argC, argV, std::move(options));
///       const auto count = variableMap.at("count").as<int>();
///   }
///
/// This function does not always return. If `--help` is present it prints the option set to stdout
/// and calls `std::exit(EXIT_SUCCESS)`. If parsing or notifier validation fails it prints the
/// error and the option set to stderr and calls `std::exit(EXIT_FAILURE)`. On success it runs all
/// registered notifiers before returning.
///
/// @param argC Number of entries in @p argV. Typically `main`'s `argc`.
///
/// @param argV Points to @p argC C-strings, including the program name at index 0. Typically
/// `main`'s `argv`. Must stay valid for the duration of the call.
///
/// @param options The option set to parse against, e.g. `RunContext::cliOptions()` extended with
/// the program's own flags. Consumed: pass a temporary or `std::move` it in.
///
/// @return The parsed variable map, with the extra `"commandLine"` entry injected.
[[nodiscard]] boost::program_options::variables_map parseCommandLine(
    int argC,
    const char* const* argV,
    boost::program_options::options_description options);

} // namespace nioc::terminus
