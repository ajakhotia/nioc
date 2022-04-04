////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

#include <naksh/logger/channelReader.hpp>

namespace naksh::logger
{
namespace bio = boost::iostreams;
using ChannelId = uint64_t;


ChannelReader::ChannelReader(std::filesystem::path logRoot):
    mLogRoot(std::move(logRoot)),
    mIndexFilePtr(std::make_shared<bio::mapped_file_source>(logRoot / kIndexFileName))
{
}


ChannelReader::iterator ChannelReader::begin() const
{
    return {mIndexFilePtr};
}


ChannelReader::iterator::iterator(std::shared_ptr<bio::mapped_file_source> indexFilePtr):
    mIndexFilePtr(std::move(indexFilePtr))
{
}


// MemoryCrate ChannelReader::iterator::operator*() const {}
//
//
// MemoryCrate* ChannelReader::iterator::operator->() const {}
//
//
// ChannelReader::iterator ChannelReader::iterator::operator++(int) {}
//
//
// ChannelReader::iterator& ChannelReader::iterator::operator++() {}
//
//
// ChannelReader::iterator ChannelReader::iterator::operator--(int) {}
//
//
// ChannelReader::iterator& ChannelReader::iterator::operator--() {}

} // namespace naksh::logger
