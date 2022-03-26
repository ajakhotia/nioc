////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"

#include <boost/endian.hpp>
#include <numeric>

namespace naksh::logger
{

bool fileHasSpace(std::ofstream& file, const size_t spaceRequired, const size_t maxFileSizeInBytes)
{
    if(spaceRequired > maxFileSizeInBytes)
    {
        throw std::invalid_argument("[logger::Channel] Space requested on a file is greater max "
                                    "the maximum allowed size of the file. This is an impossible "
                                    "constraint to satisfy.");
    }

    return (maxFileSizeInBytes - file.tellp()) >= spaceRequired;
}


size_t computeTotalSizeInBytes(const std::vector<std::span<const std::byte>>& dataCollection)
{
    return std::accumulate(
        dataCollection.begin(),
        dataCollection.end(),
        size_t(0),
        [](const size_t accumulatedSize, const std::span<const std::byte>& data) -> size_t
        { return accumulatedSize + data.size_bytes(); });
}


void writeToFile(std::ofstream& file, uint64_t integer)
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
