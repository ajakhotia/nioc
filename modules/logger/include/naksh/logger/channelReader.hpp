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

private:
    std::filesystem::path mLogRoot;

    std::shared_ptr<boost::iostreams::mapped_file_source> mIndexFile;
};


//class ChannelReader::iterator: public std::random_access_iterator_tag
//{
//public:
//    using iterator_concept = std::random_access_iterator_tag;
//
//    using value_type = MemoryCrate;
//
//    using pointer = MemoryCrate*;
//
//    using reference = MemoryCrate&;
//
//    using const_reference = const MemoryCrate&;
//
//    iterator(std::shared_ptr<boost::iostreams::mapped_file_source> indexFile, std::uint64_t index);
//
//    iterator(const iterator&) = default;
//
//    iterator(iterator&&) = default;
//
//    ~iterator() = default;
//
//    iterator& operator=(const iterator&) = default;
//
//    iterator& operator=(iterator&&) = default;
//
//    MemoryCrate operator*() const;
//
//    MemoryCrate* operator->() const;
//
//    iterator& operator++();
//
//    iterator operator++(int);
//
//    iterator& operator--();
//
//    iterator operator--(int);
//
//private:
//    std::shared_ptr<boost::iostreams::mapped_file_source> mIndexFile;
//
//    std::shared_ptr<boost::iostreams::mapped_file_source> mActiveLogRoll;
//};

} // namespace naksh::logger
