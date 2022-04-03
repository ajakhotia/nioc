////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"

#include <boost/endian.hpp>
#include <numeric>
#include <spdlog/fmt/chrono.h>

namespace naksh::logger
{

std::string timeAsFormattedString(std::chrono::system_clock::time_point timePoint)
{
    std::string timeString;

    fmt::format_to(
        std::back_inserter(timeString), "{:%Y-%m-%dT%H:%M:%S%z}", fmt::localtime(timePoint));

    return timeString;
}


std::string padString(const std::string& input, const uint64_t paddedLength, const char paddingChar)
{
    return std::string(paddedLength - std::min(paddedLength, input.size()), paddingChar) + input;
}


bool fileHasSpace(std::ofstream& file,
                  const std::uint64_t spaceRequired,
                  const std::uint64_t maxFileSizeInBytes)
{
    if(spaceRequired > maxFileSizeInBytes)
    {
        throw std::invalid_argument("[logger::Channel] Space requested on a file is greater than "
                                    "the maximum allowed size of the file. This is an impossible "
                                    "constraint to satisfy.");
    }

    return (maxFileSizeInBytes - file.tellp()) >= spaceRequired;
}


std::uint64_t computeTotalSizeInBytes(const std::vector<std::span<const std::byte>>& dataCollection)
{
    return std::accumulate(
        dataCollection.begin(),
        dataCollection.end(),
        std::uint64_t(0),
        [](const std::uint64_t accumulatedSize, const std::span<const std::byte>& data)
        { return accumulatedSize + data.size_bytes(); });
}


void writeToFile(std::ofstream& file, std::uint64_t integer)
{
    boost::endian::native_to_little_inplace(integer);

    // Suppress clang-tidy: NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file.write(reinterpret_cast<const char*>(&integer), sizeof(integer));

    // Check if the file is still good.
    if(not file.good())
    {
        throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
    }
}


void writeToFile(std::ofstream& file, const std::span<const std::byte>& data)
{
    // Suppress clang-tidy: NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file.write(reinterpret_cast<const char*>(data.data()), std::ssize(data));

    // Check if the file is still good.
    if(not file.good())
    {
        throw std::runtime_error("[Logger::utils] Unable to cleanly write to the file.");
    }
}

} // namespace naksh::logger
