////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

namespace naksh::logger
{

class Channel
{
public:
    static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

    using ConstByteSpan = std::span<const std::byte>;

    explicit Channel(std::filesystem::path logRoot,
                     std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

    Channel(const Channel&) = delete;

    Channel(Channel&&) noexcept = default;

    ~Channel() = default;

    Channel& operator=(const Channel&) = delete;

    Channel& operator=(Channel&&) noexcept = default;

    void writeFrame(const ConstByteSpan& data);

    void writeFrame(const std::vector<ConstByteSpan>& dataCollection);

private:
    std::filesystem::path mLogRoot;

    std::ofstream mIndexFile;

    std::size_t mMaxFileSizeInBytes;

    ssize_t mRollCounter;

    std::ofstream mActiveLogRoll;

    void rollAndIndex(std::size_t requiredSizeInBytes);

    std::filesystem::path nextRollFilePath();
};

} // namespace naksh::logger
