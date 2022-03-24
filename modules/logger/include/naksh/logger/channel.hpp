////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace naksh::logger
{

class Channel
{
public:
    static constexpr auto kDefaultRolloverLimitBytes = 128ULL * 1024ULL * 1024ULL;

    explicit Channel(std::filesystem::path logRoot,
                     std::size_t rollOverLimitBytes = kDefaultRolloverLimitBytes);

    Channel(const Channel&) = delete;

    Channel(Channel&&) noexcept = default;

    ~Channel() = default;

    Channel& operator=(const Channel&) = delete;

    Channel& operator=(Channel&&) noexcept = default;

    void write(size_t bufferLength, const void* bufferPtr);

private:
    std::filesystem::path mLogRoot;

    std::size_t mRollOverLimitBytes;

    std::size_t mRollCounter;

    std::ofstream mActiveLogRoll;

    void advanceLogRoll();

    bool rollHasSpace(size_t spaceRequired);
};

} // namespace naksh::logger
