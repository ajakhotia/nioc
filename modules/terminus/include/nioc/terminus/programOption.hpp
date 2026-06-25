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

/// @brief Create a fresh options description that already carries the standard `--help`/`-h` flag.
///
/// Use this to start defining a program's command-line interface, then chain `add_options()` to
/// register your own flags before handing the result to @ref parseCommandLine.
///
/// Example:
///
///     auto options = programOptions("myTool");
///     options.add_options()("count,c", boost::program_options::value<int>(), "Item count");
///
/// @param programName Names the program. Used as the title prefix in the generated help text.
///
/// @see parseCommandLine
[[nodiscard]] boost::program_options::options_description programOptions(
    const std::string& programName);

/// @brief Parse a process's argument vector against @p options and return the populated variable
/// map, handling `--help` and errors by terminating the process.
///
/// Call this once from `main` after building @p options with @ref programOptions. On success the
/// returned map also holds a `"commandLine"` string entry: every argument joined by single spaces.
///
/// Example:
///
///     int main(int argC, char** argV)
///     {
///         auto options = programOptions("myTool");
///         auto vm = parseCommandLine(argC, argV, options);
///         // vm["commandLine"].as<std::string>() == "myTool ..."
///     }
///
/// This function does not always return. If `--help` is present it prints @p options to stdout and
/// calls `std::exit(EXIT_SUCCESS)`. If parsing or notifier validation fails it prints the error and
/// @p options to stderr and calls `std::exit(EXIT_FAILURE)`. On success it runs all registered
/// notifiers before returning.
///
/// @param argC Number of entries in @p argV. Typically `main`'s `argc`.
///
/// @param argV Points to @p argC C-strings, including the program name at index 0. Typically
/// `main`'s `argv`. Must stay valid for the duration of the call.
///
/// @param options The option set to parse against. Pass the result of @ref programOptions,
/// extended with your own flags.
///
/// @return The parsed variable map, with the extra `"commandLine"` entry injected.
///
/// @see programOptions
[[nodiscard]] boost::program_options::variables_map parseCommandLine(
    int argC,
    const char* const* argV,
    const boost::program_options::options_description& options);

} // namespace nioc::terminus
