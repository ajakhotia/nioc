////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <span>

namespace naksh::logger
{


struct MemoryCrate
{
    using ChannelId = std::uint64_t;

    std::span<const std::byte> mData;

    ChannelId mChannelId;
};


class ChannelReader
{
public:
    class iterator;

    explicit ChannelReader(std::filesystem::path logRoot);

    ChannelReader(const ChannelReader&) = delete;

    ChannelReader(ChannelReader&&) = default;

    ~ChannelReader() = default;

    ChannelReader& operator=(const ChannelReader&) = delete;

    ChannelReader& operator=(ChannelReader&&) = default;

    [[nodiscard]] iterator begin() const;

private:
    std::filesystem::path mLogRoot;

    std::shared_ptr<boost::iostreams::mapped_file_source> mIndexFilePtr;
};


class ChannelReader::iterator: public std::random_access_iterator_tag
{
public:
    using iterator_concept = std::random_access_iterator_tag;

    using value_type = MemoryCrate;

    using pointer = MemoryCrate*;

    using reference = MemoryCrate&;

    using const_reference = const MemoryCrate&;

    explicit iterator(std::shared_ptr<boost::iostreams::mapped_file_source> indexFilePtr);

    iterator(const iterator&) = default;

    iterator(iterator&&) = default;

    ~iterator() = default;

    iterator& operator=(const iterator&) = default;

    iterator& operator=(iterator&&) = default;

private:
    std::shared_ptr<boost::iostreams::mapped_file_source> mIndexFilePtr;

    std::shared_ptr<boost::iostreams::mapped_file_source> mActiveLogRollPtr;
};

} // namespace naksh::logger
