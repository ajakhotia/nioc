////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/hash2/fnv1a.hpp>
#include <nioc/chronicle/defines.hpp>

namespace nioc::chronicle
{

ChannelId makeChannelId(const std::uint64_t typeId, const std::string_view topic)
{
  auto hasher = boost::hash2::fnv1a_64{typeId};
  hasher.update(topic.data(), topic.size());
  return ChannelId{hasher.result()};
}

} // namespace nioc::chronicle
