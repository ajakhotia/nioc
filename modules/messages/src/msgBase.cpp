////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <capnp/message.h>
#include <cassert>
#include <naksh/messages/msgBase.hpp>

namespace naksh::messages
{

using ConstByteSpan = std::span<const std::byte>;
using ConstWordArrayPtr = kj::ArrayPtr<const capnp::word>;
using MallocMessageBuilder = capnp::MallocMessageBuilder;

namespace
{

ConstWordArrayPtr convert(const std::span<const std::byte>& data)
{
    return {std::bit_cast<const capnp::word*>(data.data()),
            data.size() * sizeof(std::byte) / sizeof(capnp::word)};
}


ConstByteSpan convert(const ConstWordArrayPtr& data)
{
    return {std::bit_cast<const std::byte*>(data.begin()),
            data.size() * sizeof(capnp::word) / sizeof(std::byte)};
}

} // namespace


MMappedMessageReader::MMappedMessageReader(logger::MemoryCrate memoryCrate):
    MemoryCrate(std::move(memoryCrate)),
    FlatArrayMessageReader(convert(span()))
{
}


MsgBase::MsgBase(): mVariant(std::in_place_type<MallocMessageBuilder>) {}


MsgBase::MsgBase(logger::MemoryCrate memoryCrate):
    mVariant(std::in_place_type<MMappedMessageReader>, std::move(memoryCrate))
{
}


MsgBase::Variant& MsgBase::variant() noexcept
{
    return mVariant;
}


void write(MsgBase& msgBase, logger::Logger& logger)
{
    const auto segments = std::get<MallocMessageBuilder>(msgBase.variant()).getSegmentsForOutput();

    std::vector<std::uint32_t> table;
    table.reserve(segments.size() + 2);
    {
        auto inserter = std::back_inserter(table);

        // Insert the number of segments - 1 into the table.
        inserter = static_cast<uint32_t>(segments.size()) - 1;

        // Insert size of each segment. The size here is in multiple of capnp::word
        std::transform(segments.begin(),
                       segments.end(),
                       inserter,
                       [](const ConstWordArrayPtr& segment) { return segment.size(); });

        // If there are even number of segments, there will be odd number of
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
    std::transform(segments.begin(),
                   segments.end(),
                   std::back_inserter(spanCollection),
                   [](const ConstWordArrayPtr& arrayPtr) { return convert(arrayPtr); });

    logger.write(msgBase.msgHandle(), spanCollection);
}


} // namespace naksh::messages
