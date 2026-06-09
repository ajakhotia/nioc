////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <capnp/message.h>
#include <cassert>
#include <nioc/terminus/msgBase.hpp>
#include <string_view>
#include <vector>

namespace nioc::terminus
{

using ConstByteSpan = std::span<const std::byte>;
using ConstWordArrayPtr = kj::ArrayPtr<const capnp::word>;
using MallocMessageBuilder = capnp::MallocMessageBuilder;

namespace
{

ConstWordArrayPtr convert(const std::span<const std::byte>& data)
{
  return {
      std::bit_cast<const capnp::word*>(data.data()),
      data.size() * sizeof(std::byte) / sizeof(capnp::word)};
}

ConstByteSpan convert(const ConstWordArrayPtr& data)
{
  return {
      std::bit_cast<const std::byte*>(data.begin()),
      data.size() * sizeof(capnp::word) / sizeof(std::byte)};
}

} // namespace

MMappedMessageReader::MMappedMessageReader(MemoryCrate memoryCrate):
  MemoryCrate(std::move(memoryCrate)),
  FlatArrayMessageReader(convert(span()))
{
}

MsgBase::MsgBase(): mVariant(std::in_place_type<MallocMessageBuilder>) {}

MsgBase::MsgBase(chronicle::MemoryCrate memoryCrate):
  mVariant(std::in_place_type<MMappedMessageReader>, std::move(memoryCrate))
{
}

MsgBase::Variant& MsgBase::variant() noexcept
{
  return mVariant;
}

const MsgBase::Variant& MsgBase::variant() const noexcept
{
  return mVariant;
}

void write(const MsgBase& msgBase, const chronicle::ChannelId channelId, chronicle::Writer& writer)
{
  // const_cast: serialization only reads the finalized message; capnp's getSegmentsForOutput is not
  // const-qualified.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  auto& builder = const_cast<MallocMessageBuilder&>(
      std::get<MallocMessageBuilder>(msgBase.variant()));
  const auto segments = builder.getSegmentsForOutput();

  std::vector<std::uint32_t> table;
  table.reserve(segments.size() + 2);
  {
    auto inserter = std::back_inserter(table);

    // Insert the number of segments - 1 into the table.
    inserter = static_cast<uint32_t>(segments.size()) - 1;

    // Insert size of each segment. The size here is in multiple of capnp::word
    std::ranges::transform(
        segments,
        inserter,
        [](const ConstWordArrayPtr& segment) { return segment.size(); });

    // If there are even number of segments, there will be an odd number of
    // entries in the table. Append a 0 to even them out.
    if(segments.size() % 2 == 0)
    {
      inserter = 0U;
    }
    assert(table.size() % 2 == 0);
  }


  // Convert the table and the segments to span collection.
  std::vector<ConstByteSpan> spanCollection;
  spanCollection.reserve(segments.size() + 1);

  spanCollection.emplace_back(std::as_bytes(std::span(table)));
  std::ranges::transform(
      segments,
      std::back_inserter(spanCollection),
      [](const ConstWordArrayPtr& arrayPtr) { return convert(arrayPtr); });

  writer.write(channelId, spanCollection);
}

void write(const MsgBase& msgBase, const std::string_view& topic, chronicle::Writer& writer)
{
  write(msgBase, makeChannelId(msgBase.msgId(), topic), writer);
}

chronicle::ChannelId makeChannelId(const MsgId& msgId, const std::string_view& topic)
{
  // boost::hash_combine mixing: the golden-ratio constant and the two shift amounts.
  constexpr auto kGoldenRatio = std::uint64_t{0x9e3779b97f4a7c15ULL};
  constexpr auto kLeftShift = 6U;
  constexpr auto kRightShift = 2U;

  auto seed = std::hash<std::string_view>{}(topic);
  seed ^= msgId.mValue + kGoldenRatio + (seed << kLeftShift) + (seed >> kRightShift);
  return chronicle::ChannelId{seed};
}

} // namespace nioc::terminus
