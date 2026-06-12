////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <nioc/terminus/programOption.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace nioc::terminus
{
namespace po = boost::program_options;

po::options_description programOptions(const std::string& programName)
{
  auto options = po::options_description(programName + " options");
  options.add_options()("help,h", "Print this help message");
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
