////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <nioc/terminus/programOption.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nioc::terminus
{
namespace fs = std::filesystem;
namespace po = boost::program_options;

po::options_description programOptions(const std::string& programName)
{
  auto portOptions = po::options_description("port options");

  // clang-format off
  portOptions.add_options()
  (
    "log-root",
    po::value<std::string>()->default_value((fs::temp_directory_path() / "niocLogs").string()),
    "Directory under which a fresh recording is created. Created if missing. "
    "Defaults to <system-temp>/niocLogs"
  )
  (
    "append-config",
    po::value<std::vector<std::string>>(),
    "JSON config file to merge. Repeat to add more; files merge left-to-right, so a later "
    "file overrides an earlier one"
  )
  (
    "append-resource",
    po::value<std::vector<std::string>>(),
    "File to copy into the recording as a logged resource. Repeat to add more"
  )
  (
    "record-chronicle",
    po::value<bool>()->default_value(true),
    "Whether to record the chronicle time-series data stream. Pass false to skip it"
  );
  // clang-format on

  auto options = po::options_description(programName + " options");
  options.add_options()("help,h", "Print this help message");
  options.add(portOptions);
  return options;
}

po::variables_map parseCommandLine(
    const int argC,
    const char* const* const argV,
    const po::options_description& options)
{
  try
  {
    auto variableMap = po::variables_map();
    po::store(po::parse_command_line(argC, argV, options), variableMap);

    if(variableMap.contains("help"))
    {
      std::cout << options << '\n';
      std::exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe)
    }

    po::notify(variableMap);

    auto commandLine = std::ostringstream{};
    auto separator = std::string_view{};
    for(const std::string_view arg: std::span(argV, argC))
    {
      commandLine << separator << arg;
      separator = " ";
    }

    variableMap.emplace("commandLine", po::variable_value(commandLine.str(), false));
    return variableMap;
  }
  catch(const po::error& e)
  {
    std::cerr << "Error: " << e.what() << '\n' << options << '\n';
    std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
  }
}

} // namespace nioc::terminus
